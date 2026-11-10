"""EV Nova 1.0.10 for Mac OS: the application's data fork, two PEF containers built here (Classic and
Carbon) and two cut from the user's own copy (RezLib, FATCarbon), as tools/mac/regate1010.py --settled,
peflink1010.py --image --real-main and macfork1010.py --pin-dates make it (Research/mac-1.0.10-pin.md).

  compile  the stock Mac compiler plugins under mwppcemu.py (Unicorn): MW C/C++ PPC 2.4.1 b0160 for the
           game, 2.4.6 b222 for the libraries, each library on its own flag line (tools/mac/pefshare.py);
           the tree is a copy with the few edits PATCHES lists
  link     MWLinkPPC (Pro 6) per fragment, objects in sources/mac/link/objects-FRAG.txt order, import
           libraries as sources/mac/link/imports-FRAG.tsv names them
  fork     the four containers on 16-byte boundaries; the two link dates set to the original's
"""
import concurrent.futures
import re
import shutil
import struct
import sys
import tempfile
from pathlib import Path

from .host import REPO, BuildError, exe_env, run, run_exe, say, winpath

SOURCES = REPO / "sources"
LINKDIR = SOURCES / "mac" / "link"
FRAGS = ("classic", "carbon")
FRAGNAME = {"classic": "EV Nova.ppc", "carbon": "EV Nova.carbon"}
PREFIX = {"classic": "EVNovaClassic.h", "carbon": "EVNovaCarbon.h"}
DIRS = ("include", "engine", "lib", "mac/include", "mac/pef", "mac/spriteworld", "mac/port", "mac/at", "mac/tu",
        "mac/morefilesx", "mac/rt3", "mac/zlib", "mac/libpng", "mac/libjpeg", "port", "runtime")

# The game's line (b0160) and each library's (b222). A library is a file of sources/runtime or
# sources/mac/<lib> whose name its pattern matches.
GAME = "-dialect c -opt level=2,noschedule,peephole -enum min -nomapcr -Cpp_exceptions off -sym on -inline auto"
LIB_DIRS = ("runtime", "zlib", "libpng", "libjpeg", "at")
BARE = ("zlib", "libpng", "libjpeg")      # public library text: no game prefix
L4D = "-dialect c -opt level=4,schedule,peephole -inline auto,deferred -enum min -nomapcr -Cpp_exceptions off"
L4 = "-dialect c -opt level=4,schedule,peephole -enum min -nomapcr -Cpp_exceptions off"
L2 = "-dialect c -opt level=2,schedule,peephole -enum min -nomapcr -Cpp_exceptions off"
LIBS = [
    (r"^j(?!pegcrt)", L4D),                                                   # libjpeg
    (r"^z123_", L4D),                                                         # zlib
    (r"^png128_", L4D),                                                       # libpng
    (r"^(reggie|atzstream)\.c$", L2),                                         # REGGIE
    (r"^(atfilespecmac|atfilespec2mac|atfilebuffermac|atfilearchivemac|atfileplatmac)\.c$",
     "-dialect c -opt level=4,noschedule,peephole -enum min -nomapcr -Cpp_exceptions off"),   # FileTool
    (r"^(atsoundmac|atsnddrvmac|atadpcm)\.c$",
     "-dialect c -proc 750 -opt level=4,schedule,peephole -enum min -nomapcr -Cpp_exceptions off"),  # SoundTool
    (r"^(morefilesextrasmac|atsearchmac)\.c$", L2),                           # MoreFiles
    (r"^(rt3mac|rt3commonmac|rt3nagmac|rt3supermac)\.c$",
     "-dialect c -opt level=4,schedule,peephole -inline deferred -enum min -nomapcr -Cpp_exceptions off"),  # RT3
    (r"^(itatsuimac|itatsuportmac|itblit|itpagemac|convertutf|atvideomac|atvideodevmac|atvideoplatmac|atomicmac|itframemac|"
     r"itpopupmac|itbordermac|itattr|itbmp|itpng|itgif|itjpeg|itmarkup|itpict|itppat|itrect|ittga|atcharset|athashtbl|"
     r"atplatstr|atsndrate|atutf162|atnetaddr|atnetsock|atnet|atnetinfo|atnetstack|atnetdispatch|atnettcp|atnettcpstack|"
     r"atnettcpresolver|atnettcpdatagram|atnetalkstack|atnetalkresolver|atnetatalk|atnetotresolver|atnetotdatagram|"
     r"atnetotstream|atnetotstack|atcursormac|itinputmac|itbuttonmac|ittextboxmac|atplatmac|itgfxmac|itsurfmac|itindexmac|"
     r"atcritmac|attickmac|atthreadmac|ititemTU|atplatheap|atmutexlist|ittoolkit)\.c$", L4),   # AmbrosiaTools
]

# The copy differs from sources/ only here: two umbrellas include Darwin's Carbon.h, which Pro 7's headers cannot
# stand in for, and reg_tool_3.c holds two things only gcc accepts
PATCHES = [
    ("mac/morefilesx/MoreFilesX.c", "#include <Carbon.h>", "#include <EVCarbonSubset.h>"),
    ("mac/morefilesx/MoreFilesX.h", "#include <Carbon.h>", "#include <EVCarbonSubset.h>"),
    ("mac/rt3/reg_tool_3.c", 'CFStringRef paths[2] = { CFSTR(""), CFSTR("") };', "CFStringRef paths[2];"),
    ("mac/rt3/reg_tool_3.c", '#include "rt3_lib.h"', '#include "rt3_lib.h"\nvoid _RT3_RemoveFSEventCallback(void);'),
]
STUB_HEADERS = Path(__file__).resolve().parent / "mac1010_darwin.h.txt"


class Setup:
    def __init__(self, cfg, out):
        self.cfg, self.out = cfg, out
        self.tree, self.inc = out / "tree", out / "inc"
        self.b0160, self.b222 = cfg.path("mac1010", "plugin_b0160"), cfg.path("mac1010", "plugin_b222")
        self.mwlinkppc = cfg.path("mac1010", "mwlinkppc")
        self.pro63, self.pro6 = cfg.path("mac1010", "pro63"), cfg.path("mac1010", "pro6")
        self.ic_sdk, self.carbonlib_sdk = cfg.path("mac1010", "ic_sdk"), cfg.path("mac1010", "carbonlib_sdk")
        self.ui = self.pro63 / "MacOS Support" / "Universal" / "Interfaces" / "CIncludes"
        self.msl = [self.pro63 / "MSL" / p for p in ("MSL_C/MSL_Common/Include", "MSL_C/MSL_MacOS/Include",
                                                    "MSL_Extras/MSL_Common/Include", "MSL_Extras/MSL_MacOS/Include")]
        license_ = cfg.path("mac1010", "license")
        self.link_env = exe_env([cfg.path("mac1010", "flexlm326")], LM_LICENSE_FILE=winpath(license_),
                                METROWKS_LICENSE_FILE=winpath(license_), MWCIncludes="")
        self.fork = cfg.path("game", "mac1010") / "EV Nova"
        self.rsrc = cfg.path("game", "mac1010") / ".rsrc" / "EV Nova"


# -- compile ---------------------------------------------------------------------------------------
def copy_tree(s):
    shutil.rmtree(s.tree, ignore_errors=True)
    for d in DIRS:
        src = SOURCES.joinpath(*d.split("/"))
        dst = s.tree.joinpath(*d.split("/"))
        dst.mkdir(parents=True, exist_ok=True)
        for f in src.iterdir():
            if f.is_file():
                shutil.copy2(f, dst / f.name)
    for rel, old, new in PATCHES:
        p = s.tree.joinpath(*rel.split("/"))
        b = p.read_bytes()
        o, n = old.encode("latin-1"), new
        if not b.count(o):
            o, n = old.replace("\n", "\r\n").encode("latin-1"), new.replace("\n", "\r\n")
        if not b.count(o):
            raise BuildError("the tree moved: %s no longer holds %r" % (rel, old[:60]))
        p.write_bytes(b.replace(o, n.encode("latin-1")))
    # the Darwin headers the RT3 and MoreFilesX text includes, declared from Pro 7's CF headers
    text = STUB_HEADERS.read_text()
    for part in text.split("\n//// ")[1:]:
        name, body = part.split("\n", 1)
        p = s.inc.joinpath(*name.strip().split("/"))
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(body.rstrip("\n") + "\n")


def units(s):
    """{stem: source in the copy}: the umbrellas, the bare libraries, the AmbrosiaTools files, the runtime libraries."""
    t = s.tree
    found = sorted((t / "mac" / "tu").glob("*.c"))
    for d in BARE + ("at",):
        found += sorted((t / "mac" / d).glob("*.c"))
    found += [f for f in sorted((t / "runtime").glob("*.c")) if any(re.search(rx, f.name) for rx, _l in LIBS)]
    return {f.stem: f for f in found}


def is_lib(f):
    return f.parent.name in LIB_DIRS and any(re.search(rx, f.name) for rx, _l in LIBS)


def flag_line(f):
    if f.parent.name in LIB_DIRS:
        for rx, line in LIBS:
            if re.search(rx, f.name):
                return line
    return GAME


def includes_for(s, f):
    t = s.tree
    if f.parent.name in BARE:
        return [f.parent, t / "mac" / "zlib"] + s.msl + [s.ui]
    common = [s.inc, t / "include", t / "mac" / "include", t / "mac" / "rt3", t / "mac" / "morefilesx"] + s.msl + [s.ui]
    if f.name == "itpng.c":                         # it calls libpng 1.2.8; the tree's png.h is win's 1.2.7
        return [t / "mac" / "libpng", t / "mac" / "zlib"] + common
    if f.name in ("atplatheap.c", "atzstream.c"):  # they hand zlib 1.2.3's ZLIB_VERSION on
        return [t / "mac" / "zlib"] + common
    return common


def mwcppc(plugin, args, cwd):
    """One run of the Mac compiler plugin under the emulator."""
    emu = Path(__file__).resolve().parent / "mwppcemu.py"
    return run([sys.executable, emu, "--plugin", plugin] + list(args), cwd=cwd, timeout=1800)


def precompile(s, frag, lib):
    """The fragment's prefix as the emulator records it (replayed as text by every compile)."""
    plugin = s.b222 if lib else s.b0160
    pch = s.inc / ("%s-%s.pch" % (frag, "b222" if lib else "b0160"))
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / (frag + ".pch-src")
        src.write_text("#include <%s>\n" % PREFIX[frag])
        args = ["-nodefaults", "-msgstyle", "gcc", "-maxerrors", "0", "-enum", "min", "-nostdinc", "-nosyspath",
                "-i", s.tree / "mac" / "pef", "-i", s.ui]
        if lib:
            args += ["-i", s.tree / "mac" / "include"]
        for d in s.msl:
            args += ["-i", d]
        rc, msg = mwcppc(plugin, args + ["-precompile", pch, "-dialect", "c", src], tmp)
    if rc:
        raise BuildError("precompiling %s failed\n%s" % (PREFIX[frag], msg[-2000:]))
    return pch


def compile_one(s, f, pch, obj):
    lib = is_lib(f)
    args = ["-nolink", "-nodefaults", "-msgstyle", "gcc", "-maxerrors", "0"] + ("-nostdinc -nosyspath " + flag_line(f)).split()
    for d in includes_for(s, f):
        args += ["-i", d]
    args += ["-i", f.parent]
    if pch:
        args += ["-prefix", pch]
    with tempfile.TemporaryDirectory() as tmp:
        o = Path(tmp) / "o.o"
        rc, msg = mwcppc(s.b222 if lib else s.b0160, args + ["-o", o, f], tmp)
        if rc or not o.exists():
            raise BuildError("compile failed: %s\n%s" % (f.name, msg[-3000:]))
        b = o.read_bytes()
        obj.write_bytes(source_path(b, winpath(f)))
    return f.stem


def source_path(obj, path):
    """The object with its MWOB wrapper naming the source as PATH. MWLinkPPC parses that name as a Windows path
    and stops on a Unix one, so it is written as Wine shows the file. The PEF never holds it."""
    hdr = struct.unpack_from(">10I", obj, 8)
    base, body = obj[0x30:hdr[7]], obj[hdr[8]:hdr[8] + hdr[9]]
    full = path.encode("mac_roman", "replace") + b"\0"
    doff = (0x30 + len(base) + len(full) + 3) & ~3
    return (b"MWOBPPC " + struct.pack(">10I", *(hdr[:7] + (0x30 + len(base), doff, hdr[9]))) + base + full +
            bytes(doff - 0x30 - len(base) - len(full)) + body)


def listed(frag):
    return [l.split(" ", 1) for l in (LINKDIR / ("objects-%s.txt" % frag)).read_text(encoding="latin-1").splitlines() if l.strip()]


def compile_all(s):
    copy_tree(s)
    have = units(s)
    jobs = s.cfg.jobs("mac1010")
    todo = []                       # (source, prefix or None, object)
    for frag in FRAGS:
        objdir = s.out / "obj" / frag
        objdir.mkdir(parents=True, exist_ok=True)
        pchs = {False: precompile(s, frag, False), True: precompile(s, frag, True)}
        for kind, stem in listed(frag):
            if kind != "obj":
                continue
            if stem not in have:
                raise BuildError("sources/mac/link/objects-%s.txt names %s: no such unit" % (frag, stem))
            f = have[stem]
            bare = f.parent.name in BARE
            # a bare library has no prefix, so one object serves both fragments
            obj = (s.out / "obj" / "bare" / (stem + ".o")) if bare else objdir / (stem + ".o")
            if not any(t[2] == obj for t in todo):
                todo.append((f, None if bare else pchs[is_lib(f)], obj))
    (s.out / "obj" / "bare").mkdir(parents=True, exist_ok=True)
    say("  compiling %d units under the emulator (%d at a time; about %d min) ..." % (len(todo), jobs, len(todo) * 9 // (jobs * 60) + 1))
    done = 0
    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        for fut in concurrent.futures.as_completed([ex.submit(compile_one, s, *t) for t in todo]):
            fut.result()
            done += 1
            if done % 50 == 0:
                say("    %d of %d" % (done, len(todo)))


# -- link ------------------------------------------------------------------------------------------
def pef_hash(name):
    """The PEF export hash: a signed 32-bit running value, the name length in the high half."""
    v = 0
    for c in name.encode("latin-1"):
        v = (((v << 1) - (v >> 16)) ^ c) & 0xFFFFFFFF
        v -= (v & 0x80000000) << 1
    return ((len(name) << 16) | ((v ^ (v >> 16)) & 0xFFFF)) & 0xFFFFFFFF


def gen_stub(path, names):
    """A stub library: a PEF whose loader section exports NAMES [(name, class)] in that order."""
    strs, keys, syms = b"", [], []
    for n, c in names:
        keys.append(pef_hash(n))
        syms.append(struct.pack(">IIh", ((c & 0xF) << 24) | len(strs), 0, -2))
        strs += n.encode("latin-1")
    strs += b"\0" * (-len(strs) % 4)
    ld = struct.pack(">iIiIiIIIIIIIII", -1, 0, -1, 0, -1, 0, 0, 0, 0, 56, 56, 56 + len(strs), 0, len(names)) + strs
    ld += struct.pack(">I", len(names) << 18) + b"".join(struct.pack(">I", k) for k in keys) + b"".join(syms)
    head = b"Joy!peffpwpc" + struct.pack(">IIIIIHHI", 1, 0, 0, 0, 0, 1, 0, 0)
    head += struct.pack(">iIIIIIBBBB", -1, 0, 0, 0, len(ld), 80, 4, 1, 0, 0)
    path.write_bytes(head + bytes(80 - len(head)) + ld)


def import_libraries(s, frag, work):
    """MWLinkPPC tokens for the fragment's import libraries, in its library-table order."""
    tabs = []
    for line in (LINKDIR / ("imports-%s.tsv" % frag)).read_text(encoding="latin-1").splitlines():
        f = line.split("\t")
        if f[0] == "lib":
            tabs.append((f[1], int(f[2], 16), f[3] if len(f) > 3 else "gen", []))
        elif f[0] == "sym":
            tabs[-1][3].append((f[1], int(f[2], 16)))
    installs = {"cw-pro6": s.pro6, "cw-pro63": s.pro63, "ic-sdk-2.0.2": s.ic_sdk, "carbonlib-sdk-1.4": s.carbonlib_sdk}
    toks, named = [], set()
    for i, (name, opts, src, syms) in enumerate(tabs):
        path = None
        if src.startswith("fork:"):              # a library that ships inside the application: cut from the user's copy
            path = work / "forkstub" / src[5:]
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(user_parts(s)[src[5:]])
        elif src != "gen":
            kind, file_ = src.split(":", 1)
            p = installs[kind] / "MacOS Support" / "Universal" / "Libraries" / "StubLibraries" / file_
            if p.exists() and (p.parent / "RESOURCE.FRK" / file_).exists():
                path = p
            else:
                say("  NOTE: stub %s not found (%s): generated from its rows" % (src, p))
        if path is None:
            path = work / ("xstub%d" % i) / name
            path.parent.mkdir(parents=True, exist_ok=True)
            gen_stub(path, syms)
        elif path in named:
            continue
        named.add(path)
        toks += (["-wi"] if opts & 0x40 else []) + (["-ib"] if opts & 0x80 else []) + [winpath(path)]
    return toks


def link(s, frag):
    work = s.out / "link" / frag
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    libdirs = [s.pro63 / "MSL" / "MSL_C" / "MSL_MacOS" / "Lib" / "PPC",
               s.pro63 / "MacOS Support" / "Libraries" / "Runtime" / "Runtime PPC" / "Libs",
               s.pro63 / "MacOS Support" / "Universal" / "Libraries" / "PPCLibraries",
               s.pro6 / "MacOS Support" / "OpenTransport" / "Open Tpt Client Developer" / "PPC Debug Libraries"]
    inputs = []
    for kind, name in listed(frag):
        if kind == "obj":
            o = s.out / "obj" / frag / (name + ".o")
            inputs.append(o if o.exists() else s.out / "obj" / "bare" / (name + ".o"))
        elif kind == "lib":
            hit = [d / name for d in libdirs if (d / name).exists()]
            if not hit:
                raise BuildError("static library %s not found under [mac1010] pro63/pro6" % name)
            inputs.append(hit[0])
    args = ["-nodefaults", "-msgstyle", "gcc", "-xm", "a", "-macbinary", "off", "-o", "out.ppc", "-map", "out.map",
            "-m", "__start", "-fragname", FRAGNAME[frag], "-multidefs"]
    args += [winpath(p) for p in inputs] + import_libraries(s, frag, work)
    rc, log = run_exe(s.mwlinkppc, args, cwd=work, env=s.link_env, timeout=600)
    (work / "link.log").write_text(log)
    undef = sorted(set(re.findall(r"undefined '([^']+)'", log)))
    if undef:
        raise BuildError("%s link: %d undefined names, first %s" % (frag, len(undef), " ".join(undef[:8])))
    if not (work / "out.ppc").exists():
        raise BuildError("%s link wrote nothing (log: %s)\n%s" % (frag, work / "link.log", log[-2000:]))
    return (work / "out.ppc").read_bytes()


# -- the data fork ---------------------------------------------------------------------------------
# Header dates, seconds since 1904: Classic linked 2006-04-24 20:37:06, Carbon 20:40:20
STAMPS = {"classic": 0xC072E772, "carbon": 0xC072E834}
LAYOUT = [("classic", "EV Nova.ppc"), ("carbon", "EV Nova.carbon"), ("rezlib", "RezLib"), ("fatcarbon", "FATCarbon 1.5.1")]


def container_len(raw):
    """A PEF container's own length: the end of its furthest section."""
    n = struct.unpack_from(">H", raw, 32)[0]
    return max(sum(struct.unpack_from(">II", raw, 40 + 28 * i + 16)) for i in range(n))


def read_rsrc(path, typ, rid):
    """One resource's data out of a resource fork file, or None."""
    if not path.exists():
        return None
    r = path.read_bytes()
    doff, moff = struct.unpack_from(">II", r, 0)
    tl = moff + struct.unpack_from(">H", r, moff + 24)[0]
    for i in range(struct.unpack_from(">H", r, tl)[0] + 1):
        t, n, ro = struct.unpack_from(">4sHH", r, tl + 2 + 8 * i)
        for j in range(n + 1 if t == typ else 0):
            i_d, _nm, ad = struct.unpack_from(">hhI", r, tl + ro + 12 * j)
            if i_d == rid:
                o = doff + (ad & 0xFFFFFF)
                return r[o + 4:o + 4 + struct.unpack_from(">I", r, o)[0]]
    return None


def user_parts(s):
    """{name: container} of the user's copy, placed by its cfrg 0 resource, else by scanning for PEF headers."""
    d = s.fork.read_bytes()
    c = read_rsrc(s.rsrc, b"cfrg", 0)
    if c:
        mem, p = {}, 32
        for _ in range(struct.unpack_from(">H", c, 30)[0]):
            _use, _where, off, ln = struct.unpack_from(">BBII", c, p + 22)
            name = c[p + 43:p + 43 + c[p + 42]].decode("latin-1")
            mem[name] = (off, ln or len(d) - off)
            p += struct.unpack_from(">H", c, p + 40)[0]
    else:
        offs = [m.start() for m in re.finditer(b"Joy!peff", d)]
        mem = {LAYOUT[i][1]: (o, (offs[i + 1] if i + 1 < len(offs) else len(d)) - o) for i, o in enumerate(offs)}
    out = {}
    for _k, name in LAYOUT[2:]:
        off, ln = mem[name]
        out[name] = d[off:off + container_len(d[off:off + ln])]
    return out


def build(cfg, out, pin_date=True):
    s = Setup(cfg, out)
    say("  [1/3] compile")
    compile_all(s)
    parts = {}
    for frag in FRAGS:
        say("  [2/3] link %s" % frag)
        raw = link(s, frag)
        if pin_date:
            raw = raw[:16] + struct.pack(">I", STAMPS[frag]) + raw[20:]
        parts[FRAGNAME[frag]] = raw[:container_len(raw)]
    say("  [3/3] data fork")
    parts.update(user_parts(s))
    fork = b""
    for _k, name in LAYOUT:
        fork += bytes(-len(fork) % 16) + parts[name]
    dst = out / "EV Nova"
    dst.write_bytes(fork)
    return dst
