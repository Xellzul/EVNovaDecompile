"""EV Nova 1.1.1 for Mac OS X: the Universal (i386 + ppc) binary EV Nova.app/Contents/MacOS/EV Nova, as
tools/mac/maclink.py makes it (Research/mac-link.md).

  compile  every sources/mac/tu umbrella with Apple gcc 4.0.1 (build 5494) to assembly, per slice, the prefix
           header precompiled as Xcode did; cc1 runs with the Darwin qsort preloaded and without address
           randomisation (its sort ties and its PCH address are host facts)
  names    two rewrites read from the user's image, names only (tools/mac/macprep.py): the statics' numbers
           (name.<uid>, the declaration counts of headers we do not have) and the debug map's source paths;
           then cctools' as
  link     ld64-97.17 with the original's command line, every input at the path the original link saw
           (/Users/jerome/..., /Developer), RT3 as a fat librt3_nonag.a; the slices' code signature copied
           from the user's image, lipo -create
"""
import collections
import concurrent.futures
import hashlib
import os
import re
import shutil
import struct
import tempfile
import threading
from pathlib import Path

from . import umbrellas
from .host import REPO, BuildError, run, say

TU_DIR = REPO / "sources" / "mac" / "tu"
PREFIX = REPO / "sources" / "mac" / "include" / "EVNovaPrefix.h"
ARCH = {"i386": "i686", "ppc": "powerpc"}
SLICES = ("i386", "ppc")
JEROME = "/Users/jerome/Ambrosia/ev_nova1.1.0/sources/3rdparty/ev_nova/"
OBJDIR = JEROME + "build/EV_Nova.build/Release/Nova.build/Objects-normal/"
RELEASE = JEROME + "build/Release/"
DEV_SDK = "/Developer/SDKs/MacOSX10.4u.sdk"
FRAMEWORKS = ["Carbon", "Cocoa", "AmbrosiaTools", "ASWDebug", "ShortcutRecorder", "ApplicationServices",
              "QuickTime", "Security", "SystemConfiguration"]


# -- reading a Mach-O image ------------------------------------------------------------------------
class MachO:
    """A thin 32-bit Mach-O: load commands and the symbol table."""

    def __init__(self, data):
        self.data = data
        self.e = {0xFEEDFACE: "<", 0xCEFAEDFE: ">"}.get(struct.unpack("<I", data[:4])[0])
        if not self.e:
            raise BuildError("not a thin 32-bit Mach-O")
        _m, _cpu, _sub, _ft, self.ncmds, self.sizeofcmds, _fl = struct.unpack_from(self.e + "7I", data, 0)
        self.lcs, off = [], 28
        for _ in range(self.ncmds):
            cmd, size = struct.unpack_from(self.e + "2I", data, off)
            self.lcs.append((cmd, size, off, data[off:off + size]))
            off += size

    def symtab(self):
        """[(name, type, value)] of the nlist table."""
        cmd = next(raw for c, _s, _o, raw in self.lcs if c == 0x2)
        symoff, nsyms, stroff, _ss = struct.unpack_from(self.e + "4I", cmd, 8)
        out = []
        for i in range(nsyms):
            strx, typ, _sect, _desc, value = struct.unpack_from(self.e + "IBBHI", self.data, symoff + 12 * i)
            q = stroff + strx
            out.append((self.data[q:self.data.index(b"\0", q)].decode("latin-1"), typ, value))
        return out

    def linkedit(self):
        return next(o for c, _s, o, raw in self.lcs if c == 0x1 and raw[8:24].split(b"\0")[0] == b"__LINKEDIT")


def slices(path):
    """{"i386": MachO, "ppc": MachO} of a fat file."""
    d = Path(path).read_bytes()
    if d[:4] != b"\xca\xfe\xba\xbe":
        raise BuildError("%s is not a Universal binary" % path)
    out = {}
    for i in range(struct.unpack_from(">I", d, 4)[0]):
        cpu, _sub, off, size, _al = struct.unpack_from(">5I", d, 8 + 20 * i)
        out[{7: "i386", 18: "ppc"}[cpu]] = MachO(d[off:off + size])
    return out


def resign(unsigned, signed):
    """Graft the signed image's LC_CODE_SIGNATURE and signature blob onto our unsigned slice. The blob's
    hashes are the original's bytes': the result is valid only if the slice is identical."""
    u = MachO(unsigned)
    d, e = bytearray(unsigned), u.e
    _c, size, _o, raw = next(x for x in signed.lcs if x[0] == 0x1D)
    dataoff, datasize = struct.unpack_from(signed.e + "2I", raw, 8)
    at = (len(d) + 15) & ~15
    d += b"\0" * (at - len(d))
    lc = 28 + u.sizeofcmds
    if any(d[lc:lc + size]):
        raise BuildError("no room for LC_CODE_SIGNATURE")
    d[lc:lc + size] = struct.pack(e + "4I", 0x1D, size, at, datasize)
    struct.pack_into(e + "2I", d, 16, u.ncmds + 1, u.sizeofcmds + size)
    le = u.linkedit()
    vmaddr, _vmsize, fileoff, _filesize = struct.unpack_from(e + "4I", d, le + 24)
    filesize = at + datasize - fileoff
    struct.pack_into(e + "4I", d, le + 24, vmaddr, (filesize + 0xFFF) & ~0xFFF, fileoff, filesize)
    return bytes(d) + signed.data[dataoff:dataoff + datasize]


# -- compile ---------------------------------------------------------------------------------------
class Setup:
    def __init__(self, cfg, out):
        self.out = out
        self.gcc = cfg.path("mac111", "applegcc")
        self.sdk = Path(cfg.get("mac111", "sdk"))              # as written: the rewrite below matches its text
        self.xcode = cfg.path("mac111", "xcode")
        self.frameworks = cfg.path("mac111", "frameworks")
        self.work = cfg.path("mac111", "workdir")
        self.jobs = cfg.jobs("mac111")
        self.image = cfg.path("game", "mac111")
        self.qsort = self.gcc / "lib" / "darwin_qsort.so"
        self.pch_lock = threading.Lock()
        self.pchs = {}

    def cflags(self, sl):
        """The image's compile line beyond -Os -mdynamic-no-pic: the 10.4 target, hidden visibility and DWARF (the
        debug map), ppc's G5 scheduling, Xcode 3's front-end flags, the prefix header, the 10.4u SDK."""
        f = ["-mmacosx-version-min=10.4", "-fvisibility=hidden", "-gdwarf-2"] + (["-mtune=G5"] if sl == "ppc" else [])
        f += ["-std=gnu99", "-fasm-blocks", "-fpascal-strings", "-DV111", "-DMACBUILD",
              "-include", str(PREFIX), "-I" + str(REPO / "sources" / "include")]
        return f + ["-isysroot", str(self.sdk), "-isystem", "%s/usr/lib/gcc/%s-apple-darwin10/4.0.1/include" % (self.sdk, ARCH[sl]),
                    "-iwithsysroot", "/usr/include"]

    def cc1(self, cmd, cwd):
        """Run the gcc driver as the identical builds did: Darwin's qsort preloaded, no ASLR (setarch -R)."""
        env = dict(os.environ, LD_PRELOAD=str(self.qsort))
        return run(["setarch", "x86_64", "-R"] + [str(c) for c in cmd], cwd=cwd, env=env, timeout=900)


def umbrella_flags(src):
    """A `// MAC-CFLAGS:` line in the umbrella or any file it includes: a library's own compile line (RT3)."""
    seen, todo = [], [src.resolve()]
    while todo:
        f = todo.pop(0)
        if f in seen or not f.exists():
            continue
        seen.append(f)
        text = f.read_text(encoding="latin-1")
        m = re.search(r"^\s*//\s*MAC-CFLAGS:\s*(.*?)\s*$", text, re.M)
        if m:
            return m.group(1).split()
        todo += [(f.parent / inc).resolve() for inc in re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.M)]
    return []


def pch(s, sl, extra):
    """The prefix header precompiled for this slice and these extra flags (one directory per text and line)."""
    flags = s.cflags(sl)
    i = flags.index(str(PREFIX))
    rest = flags[:i - 1] + flags[i + 1:] + [f for f in extra if not f.startswith(("-I", "-iquote", "-isystem"))]
    key = hashlib.sha1(PREFIX.read_bytes() + "\0".join([sl] + rest).encode()).hexdigest()[:12]
    d = s.work / "pch" / ("%s-%s" % (sl, key))
    with s.pch_lock:
        if not (d / "EVNovaPrefix.h.gch").exists():
            shutil.rmtree(d, ignore_errors=True)
            d.mkdir(parents=True)
            shutil.copyfile(PREFIX, d / "EVNovaPrefix.h")
            b = s.gcc / ARCH[sl] / "bin"
            rc, msg = s.cc1([b / "xgcc", "-B%s/" % b, "-Os", "-mdynamic-no-pic"] + rest +
                            ["-x", "c-header", d / "EVNovaPrefix.h", "-o", d / "EVNovaPrefix.h.gch"], cwd=d)
            if rc or not (d / "EVNovaPrefix.h.gch").exists():
                shutil.rmtree(d, ignore_errors=True)
                raise BuildError("precompiling the prefix (%s) failed\n%s" % (sl, msg[-2000:]))
    return d


def compile_one(s, src, sl, outdir):
    """SRC to assembly, outdir/<stem>_<slice>.s."""
    extra = umbrella_flags(src)
    flags = s.cflags(sl)
    if src.suffix == ".m":          # Objective-C: the objc driver, the prefix read as text
        b = s.gcc / ARCH[sl] / "bin-objc"
        flags += ["-x", "objective-c"] + extra
        d = None
    else:
        b = s.gcc / ARCH[sl] / "bin"
        d = pch(s, sl, extra)
        flags[flags.index(str(PREFIX))] = str(d / "EVNovaPrefix.h")
        flags += ["-Winvalid-pch", "-H"] + extra
    dst = outdir / ("%s_%s.s" % (src.stem, sl))
    with tempfile.TemporaryDirectory() as tmp:     # gcc writes its dump files beside its working directory
        rc, msg = s.cc1([b / "xgcc", "-B%s/" % b, "-Os", "-mdynamic-no-pic"] + flags + ["-S", src, "-o", Path(tmp) / "x.s"], cwd=tmp)
        if rc or not (Path(tmp) / "x.s").exists():
            raise BuildError("compile failed: %s (%s)\n%s" % (src.name, sl, msg[-3000:]))
        if d and ("! %s/EVNovaPrefix.h.gch" % d) not in msg:
            raise BuildError("%s (%s): the precompiled prefix was not used" % (src.name, sl))
        shutil.copyfile(Path(tmp) / "x.s", dst)


# -- the two name rewrites -------------------------------------------------------------------------
UID = re.compile(r"(_[A-Za-z]\w*)\.(\d+)\b")
CGEN = re.compile(r"_C\.\d+\.\d+\b")


def image_objects(im):
    """The image's debug map per object: {object: {so, stsym, fun}} in N_OSO order, and the header names under /Users/."""
    objs, cur, so = collections.OrderedDict(), None, None
    syms = im.symtab()
    for name, typ, _v in syms:
        if typ == 0x64 and name:
            so = name
        elif typ == 0x66:
            m = re.match(r".*\((.*)\)$", name)
            cur = m.group(1) if m else name.rsplit("/", 1)[-1]
            objs[cur] = dict(so=so, stsym=[])
        elif typ == 0x26 and cur:
            objs[cur]["stsym"].append(name)
    sols = {n.rsplit("/", 1)[-1]: n for n, typ, _v in syms if typ == 0x84 and n.startswith("/Users/")}
    return objs, sols


def rename_statics(txt, stsym):
    """name.<uid> and _C.<n>.<uid> take the image's numbers, by base name in uid order on both sides."""
    uid = lambda n: int(n.rsplit(".", 1)[1])  # noqa: E731
    theirs, their_c = collections.defaultdict(list), []
    for n in stsym:
        if CGEN.fullmatch(n):
            their_c.append(n)
        elif UID.fullmatch(n):
            theirs[UID.fullmatch(n).group(1)].append(n)
    ours = collections.defaultdict(set)
    for m in UID.finditer(txt):
        if not CGEN.fullmatch(m.group(0)):
            ours[m.group(1)].add(m.group(0))
    ren = {}
    for base, names in ours.items():
        ren.update(zip(sorted(names, key=uid), sorted(theirs[base], key=uid)))
    ren.update(zip(sorted(set(CGEN.findall(txt)), key=uid), sorted(their_c, key=uid)))
    ren = {k: v for k, v in ren.items() if k != v}
    if ren:
        pat = re.compile("|".join(re.escape(k) for k in sorted(ren, key=len, reverse=True)))
        txt = pat.sub(lambda m: ren[m.group(0)], txt)
    return txt


def rewrite_paths(s, txt, so, sols):
    """The compile unit's name and the line table's directories and files, as the original's debug map names them:
    the umbrella is our construction, the original's unit was one file at another path."""
    mine = str(REPO) + "/"
    sdk_dir = str(s.sdk.parent) + "/"
    d, f = so.rsplit("/", 1)
    lines = txt.split("\n")
    for i, ln in enumerate(lines):           # DW_AT_name: the first absolute path under sources/
        m = re.match(r'\s*\.ascii "(%ssources/[^"]*)\\0"$' % re.escape(mine), ln)
        if m:
            lines[i] = '\t.ascii "%s\\0"' % so
            j = i                             # gcc wrote the unit's length as a literal: follow the new name's length
            while j > 0 and not re.match(r"^\t\.long\t0x[0-9a-f]+$", lines[j]):
                j -= 1
            if lines[j + 1].strip() in (".word\t0x2", ".short\t0x2"):
                lines[j] = "\t.long\t0x%x" % (int(lines[j].split("0x")[1], 16) + len(so) - len(m.group(1)))
            break
    try:
        a, b = lines.index("LASLTP0:"), lines.index("LELTP0:")
    except ValueError:
        return "\n".join(lines)
    head = lines[a:b]
    k = 0
    while k < len(head) and not head[k].lstrip().startswith(".ascii"):
        k += 1
    pre, rest = head[:k], head[k:]
    dirs, i = [], 0
    while i < len(rest) and rest[i].lstrip().startswith(".ascii"):
        dirs.append(re.match(r'\s*\.ascii "(.*)"$', rest[i]).group(1))
        i += 2
    after = rest[i:]
    is_src = [x.startswith(mine) for x in dirs]
    newdirs = []
    for x in dirs:
        if x.startswith(sdk_dir):
            x = re.sub(r"/Headers/\.\./Frameworks/", "/Frameworks/", x.replace(sdk_dir, "/Developer/SDKs/", 1))
        newdirs.append(x)
    my = len(dirs) + 1
    extra = sorted({v.rsplit("/", 1)[0] for v in sols.values()})
    out = list(pre)
    for x in newdirs + [d] + extra:
        out += ['\t.ascii "%s"' % x, "\t.byte\t0"]
    i = 0
    if after and after[0].lstrip().startswith(".byte"):
        out.append(after[0])
        i = 1
    while i < len(after):
        m = re.match(r'\s*\.ascii "(.*)\\0"$', after[i])
        if not m:
            out.append(after[i])
            i += 1
            continue
        name, dirline = m.group(1), after[i + 1]
        dm = re.search(r"(?:\.byte\s+|\.uleb128\s+)(0x[0-9a-f]+|\d+)", dirline)
        di = int(dm.group(1), 0) if dm else 0
        ours = di == 0 or (0 < di <= len(is_src) and is_src[di - 1])
        base = name.rsplit("/", 1)[-1]
        if ours and base in sols:
            out.append('\t.ascii "%s\\0"' % base)
            out.append(re.sub(r"(0x[0-9a-f]+|\d+)", str(my + 1 + extra.index(sols[base].rsplit("/", 1)[0])), dirline, count=1))
        elif ours:
            out.append('\t.ascii "%s\\0"' % f)
            out.append(re.sub(r"(0x[0-9a-f]+|\d+)", str(my), dirline, count=1))
        else:
            out += [after[i], dirline]
        i += 2
    return "\n".join(lines[:a] + out + lines[b:])


def umbrella_objects():
    """{image object: umbrella stem}: which object each umbrella builds (umbrellas.py)."""
    return umbrellas.check(str(TU_DIR))


# -- link ------------------------------------------------------------------------------------------
def symlink(link, target):
    link = Path(link)
    if link.is_symlink() and os.readlink(str(link)) != str(target):
        link.unlink()
    if not link.exists() and not link.is_symlink():
        link.parent.mkdir(parents=True, exist_ok=True)
        link.symlink_to(target)


def link(s, objdir, images, signed):
    tools = s.gcc / "cctools" / "bin"
    symlink(DEV_SDK, s.sdk)
    for sl in SLICES:
        t = "%s-apple-darwin10" % ARCH[sl]
        symlink("/Developer/usr/lib/gcc/%s/4.0.1" % t, s.xcode / "gcc4.0" / "usr" / "lib" / "gcc" / t / "4.0.1")
        Path(OBJDIR + sl).mkdir(parents=True, exist_ok=True)
    Path(RELEASE).mkdir(parents=True, exist_ok=True)
    # the three private frameworks, rebuilt as bundles from their binaries (an extract of the dmg may not keep the links)
    fw = s.work / "Frameworks"
    shutil.rmtree(fw, ignore_errors=True)
    for name in ("AmbrosiaTools", "ASWDebug", "ShortcutRecorder"):
        binary = "%s.framework/Versions/A/%s" % (name, name)
        (fw / binary).parent.mkdir(parents=True)
        shutil.copyfile(s.frameworks / binary, fw / binary)
        (fw / ("%s.framework" % name) / name).symlink_to("Versions/A/" + name)
    # the dmg's ShortcutRecorder names itself @loader_path; the image was linked against one that said @executable_path
    sr = "ShortcutRecorder.framework/Versions/A/ShortcutRecorder"
    rc, msg = run([tools / "install_name_tool", "-id", "@executable_path/../Frameworks/" + sr, fw / sr])
    if rc:
        raise BuildError("install_name_tool failed: " + msg)
    lnk = s.work / "link"
    shutil.rmtree(lnk, ignore_errors=True)
    lnk.mkdir(parents=True)
    rt3s, cmds = [], {}
    for sl in SLICES:
        osos = [(n, v) for n, typ, v in images[sl].symtab() if typ == 0x66]
        files, members = [], []
        for path, mtime in osos:          # the inputs in the original's order, at its paths, with its mtimes
            m = re.match(r"(.*)\((.*)\)$", path)
            if m:
                if m.group(1).endswith("librt3_nonag.a"):
                    members.append((m.group(2), mtime))
                continue
            src = objdir / sl / path.rsplit("/", 1)[-1]
            if not src.exists():
                raise BuildError("%s: no object for %s" % (sl, path))
            shutil.copyfile(src, path)
            os.utime(path, (mtime, mtime))
            files.append(path)
        (lnk / ("%s.LinkFileList" % sl)).write_text("\n".join(files) + "\n")
        # librt3_nonag.a: members in the order of their code in the image (ld gives a member its place in the file)
        first, cur = {}, None
        for name, typ, value in images[sl].symtab():
            if typ == 0x66:
                m = re.match(r".*\((.*)\)$", name)
                cur = m.group(1) if m and "librt3" in name else None
            elif typ == 0x24 and name and cur and cur not in first:
                first[cur] = value
        members.sort(key=lambda m: first.get(m[0], 1 << 30))
        ar = lnk / ("rt3-" + sl)
        ar.mkdir()
        paths = []
        for name, mtime in members:
            p = ar / name
            shutil.copyfile(objdir / sl / name, p)
            os.utime(str(p), (mtime, mtime))         # libtool records each member's mtime; ld copies it into the debug map
            paths.append(p)
        rc, msg = run([tools / "libtool", "-static", "-o", ar / "librt3_nonag.a"] + paths)
        if rc:
            raise BuildError("libtool failed: " + msg)
        rt3s.append(ar / "librt3_nonag.a")
        gcc = "/Developer/usr/lib/gcc/%s-apple-darwin10/4.0.1" % ARCH[sl]
        cmds[sl] = ([s.gcc / "ld64" / "ld", "-arch", sl, "-macosx_version_min", "10.4", "-syslibroot", DEV_SDK,
                     "-o", lnk / ("EV Nova." + sl), "-L" + RELEASE.rstrip("/"), "-L" + gcc, "-F" + str(fw), "-lcrt1.o",
                     "-filelist", lnk / ("%s.LinkFileList" % sl)] +
                    sum((["-framework", f] for f in FRAMEWORKS), []) + ["-lrt3_nonag", "-lgcc_s.10.4", "-lgcc", "-lSystem"])
    rc, msg = run([tools / "lipo", "-create"] + rt3s + ["-output", RELEASE + "librt3_nonag.a"])
    if rc:
        raise BuildError("lipo (librt3_nonag.a) failed: " + msg)
    thin = []
    for sl in SLICES:
        rc, msg = run(cmds[sl], cwd=lnk)
        (lnk / ("%s.log" % sl)).write_text(msg)
        if rc:
            raise BuildError("ld (%s) failed\n%s" % (sl, msg[-2000:]))
        f = lnk / ("EV Nova." + sl)
        if signed:
            g = lnk / ("EV Nova.%s.signed" % sl)
            g.write_bytes(resign(f.read_bytes(), images[sl]))
            f = g
        thin.append(f)
    fat = s.out / "EV Nova"
    rc, msg = run([tools / "lipo", "-create"] + thin + ["-output", fat])
    if rc:
        raise BuildError("lipo failed: " + msg)
    return fat


def build(cfg, out, signed=True):
    out.mkdir(parents=True, exist_ok=True)
    s = Setup(cfg, out)
    s.work.mkdir(parents=True, exist_ok=True)
    images = slices(s.image)
    umb = umbrella_objects()
    srcs = sorted(p for p in TU_DIR.iterdir() if p.suffix in (".c", ".m"))
    asm = s.work / "s"
    shutil.rmtree(asm, ignore_errors=True)
    asm.mkdir(parents=True)
    say("  [1/3] compile: %d umbrellas x 2 slices (%d at a time) ..." % (len(srcs), s.jobs))
    with concurrent.futures.ThreadPoolExecutor(s.jobs) as ex:
        for f in [ex.submit(compile_one, s, src, sl, asm) for sl in SLICES for src in srcs]:
            f.result()
    say("  [2/3] names from the image, assemble")
    objdir = s.work / "obj"
    shutil.rmtree(objdir, ignore_errors=True)
    for sl in SLICES:
        (objdir / sl).mkdir(parents=True)
        objs, sols = image_objects(images[sl])
        for o, info in objs.items():
            if o not in umb:
                continue
            txt = (asm / ("%s_%s.s" % (umb[o], sl))).read_text(errors="surrogateescape")
            txt = rewrite_paths(s, rename_statics(txt, info["stsym"]), info["so"], sols)
            p = asm / ("%s.%s.s" % (o, sl))
            p.write_text(txt, errors="surrogateescape")
            rc, msg = run([s.gcc / "cctools" / "libexec" / "as" / sl / "as", "-arch", sl, p, "-o", objdir / sl / o])
            if rc:
                raise BuildError("as failed on %s (%s): %s" % (o, sl, msg[-1000:]))
    say("  [3/3] link (ld64, lipo)")
    return link(s, objdir, images, signed)
