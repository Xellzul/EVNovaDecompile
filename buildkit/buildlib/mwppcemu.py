#!/usr/bin/env python3
r"""mwppcemu.py -- run a stock Mac "MW C/C++ PPC" IDE compiler plugin (PEF, PowerPC) from the command line, under Unicorn.

    python tools/mac/mwppcemu.py --plugin DIR -nolink [-i dir]... [flags] -o obj.o src.c
    python tools/mac/mwppcemu.py --plugin DIR -precompile out.pch [-i dir]... prefix-src    # recorded, replayed as text by -prefix

DIR holds `MW C++ PPC` and `MW C++ PPC.rsrc`, the plugin's two forks, unmodified, and that install's `license.dat`. This file is a stand-in for the IDE: a PEF
loader, the plugin API (PluginLib4) and the few CarbonLib calls it imports. Flags take MWCPPC.exe's spelling; only what the
IDE's panels hold is accepted, an unknown option is an error. Panel layouts, the handle model and the object wrapper follow
EvOverrideDecompile/tools/mwcppc22; Mac structs from the SDK on the Pro 6 image (mac68k alignment, big-endian, 70-byte FSSpec).
Check: the 2.4 b0131 plugin here against stock MWCPPC.exe b0131 by `pefshare.py --corpus` and tools/mac/corpuscmp.py. Research/tooling-reference.md.
"""
import json
import os
import struct
import sys
import time

from unicorn import Uc, UcError, UC_ARCH_PPC, UC_MODE_32, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.ppc_const import (UC_PPC_REG_0, UC_PPC_REG_LR, UC_PPC_REG_PC, UC_PPC_REG_MSR, UC_PPC_REG_FPR0)

REG = lambda n: UC_PPC_REG_0 + n
STUB, STUB_SIZE = 0x0f000000, 0x10000
BASE = 0x10000000
RAM = 0x30000000        # 768 MB, lazily committed by the host OS
RET = STUB + STUB_SIZE - 4
FSSPEC = 70
PFX_MAGIC = b"MWPPCEMU-PREFIX\n"


class CompileFailed(Exception):
    pass


# ---------------------------------------------------------------- PEF

def pattern_expand(p, total):
    out = bytearray()
    i = 0

    def arg():
        nonlocal i
        v = 0
        while True:
            c = p[i]
            i += 1
            v = (v << 7) | (c & 0x7f)
            if not c & 0x80:
                return v
    while i < len(p):
        c = p[i]
        i += 1
        op, n = c >> 5, c & 0x1f
        if n == 0:
            n = arg()
        if op == 0:
            out += bytes(n)
        elif op == 1:
            out += p[i:i + n]
            i += n
        elif op == 2:
            rep = arg() + 1
            out += p[i:i + n] * rep
            i += n
        elif op == 3:
            custom, rep = arg(), arg()
            common = p[i:i + n]
            i += n
            out += common
            for _ in range(rep):
                out += p[i:i + custom]
                i += custom
                out += common
        elif op == 4:
            custom, rep = arg(), arg()
            out += bytes(n)
            for _ in range(rep):
                out += p[i:i + custom]
                i += custom
                out += bytes(n)
        else:
            raise ValueError("pattern op %d" % op)
    out += bytes(max(0, total - len(out)))
    return out


class Pef:
    def __init__(self, b):
        if b[:12] != b"Joy!peffpwpc":
            raise SystemExit("not a PowerPC PEF")
        n = struct.unpack_from(">H", b, 32)[0]
        self.secs = []
        for i in range(n):
            _, addr, total, unp, pack, off, kind, share, align, _r = struct.unpack_from(">iIIIIIBBBB", b, 40 + 28 * i)
            self.secs.append(dict(total=total, pack=pack, off=off, kind=kind, unp=unp))
        self.b = b
        self.loader = next(b[s["off"]:s["off"] + s["pack"]] for s in self.secs if s["kind"] == 4)
        (self.main_s, self.main_o, self.init_s, self.init_o, self.term_s, self.term_o, self.nlib, self.nimp, self.nrel,
         self.relo, self.strs, _h, _hp, _ne) = struct.unpack_from(">iIiIiIIIIIIIII", self.loader, 0)

    def cstr(self, o):
        l = self.loader
        e = l.index(b"\0", self.strs + o)
        return l[self.strs + o:e].decode("mac_roman")

    def imports(self):
        base = 56 + 24 * self.nlib
        return [self.cstr(struct.unpack_from(">I", self.loader, base + 4 * j)[0] & 0xffffff) for j in range(self.nimp)]

    def image(self, s):
        raw = self.b[s["off"]:s["off"] + s["pack"]]
        if s["kind"] == 2:
            return pattern_expand(raw, s["total"])
        return bytearray(raw) + bytes(max(0, s["total"] - len(raw)))

    def relocate(self, images, bases, imp):
        """Apply the loader's relocations in place; images is section index -> bytearray."""
        l = self.loader
        hdr = 56 + 24 * self.nlib + 4 * self.nimp
        for r in range(self.nrel):
            sec, _x, count, first = struct.unpack_from(">HHII", l, hdr + 12 * r)
            img = images[sec]
            ins = [struct.unpack_from(">H", l, self.relo + first + 2 * k)[0] for k in range(count)]
            pos, idx, c, d = 0, 0, bases[0], bases[1]

            def add(p, v):
                struct.pack_into(">I", img, p, (struct.unpack_from(">I", img, p)[0] + v) & 0xffffffff)

            def run(lo, hi):
                nonlocal pos, idx, c, d
                k = lo
                while k < hi:
                    w = ins[k]
                    if w >> 14 == 0:            # BySectD with skip
                        pos += ((w >> 6) & 0xff) * 4
                        for _ in range(w & 0x3f):
                            add(pos, d)
                            pos += 4
                    elif w >> 13 == 2:          # run
                        sub, n = (w >> 9) & 0xf, (w & 0x1ff) + 1
                        for _ in range(n):
                            if sub == 0:
                                add(pos, c)
                                pos += 4
                            elif sub == 1:
                                add(pos, d)
                                pos += 4
                            elif sub == 2:
                                add(pos, c)
                                add(pos + 4, d)
                                pos += 12
                            elif sub == 3:
                                add(pos, c)
                                add(pos + 4, d)
                                pos += 8
                            elif sub == 4:
                                add(pos, d)
                                pos += 8
                            elif sub == 5:
                                add(pos, imp[idx])
                                idx += 1
                                pos += 4
                            else:
                                raise ValueError("reloc run sub %d" % sub)
                    elif w >> 13 == 3:          # small
                        sub, ix = (w >> 9) & 0xf, w & 0x1ff
                        if sub == 0:
                            add(pos, imp[ix])
                            pos += 4
                            idx = ix + 1
                        elif sub == 1:
                            c = bases[ix]
                        elif sub == 2:
                            d = bases[ix]
                        elif sub == 3:
                            add(pos, bases[ix])
                            pos += 4
                        else:
                            raise ValueError("reloc small sub %d" % sub)
                    elif w >> 12 == 8:          # incr position
                        pos += (w & 0xfff) + 1
                    elif w >> 12 == 9:          # small repeat
                        blocks, rep = ((w >> 8) & 0xf) + 1, (w & 0xff) + 1
                        for _ in range(rep):
                            run(k - blocks, k)
                    elif w >> 10 == 0x28:       # set position
                        k += 1
                        pos = ((w & 0x3ff) << 16) | ins[k]
                    elif w >> 10 == 0x29:       # large by import
                        k += 1
                        ix = ((w & 0x3ff) << 16) | ins[k]
                        add(pos, imp[ix])
                        pos += 4
                        idx = ix + 1
                    elif w >> 10 == 0x2c:       # large repeat
                        k += 1
                        blocks, rep = ((w >> 6) & 0xf) + 1, (((w & 0x3f) << 16) | ins[k]) + 1
                        for _ in range(rep):
                            run(k - 1 - blocks, k - 1)
                    elif w >> 10 == 0x2d:       # large set or by section
                        k += 1
                        sub, ix = (w >> 6) & 0xf, ((w & 0x3f) << 16) | ins[k]
                        if sub == 0:
                            add(pos, bases[ix])
                            pos += 4
                        elif sub == 1:
                            c = bases[ix]
                        elif sub == 2:
                            d = bases[ix]
                        else:
                            raise ValueError("reloc large sub %d" % sub)
                    else:
                        raise ValueError("reloc opcode 0x%04x" % w)
                    k += 1
            run(0, len(ins))


# ---------------------------------------------------------------- panels (host.c's layouts, big-endian)

FE = dict(CPLUSPLUS=2, CHECKPROTOS=3, ARM=4, TRIGRAPHS=5, ONLYSTD=6, ENUMINT=7, MPWPTR=8, PREFIX=9, ANSISTRICT=0x29, MAPCR=0x2a,
          WCHAR=0x2b, EXC=0x2c, NOREUSE=0x2d, POOL=0x2e, NOINLINE=0x2f, RTTI=0x30, MULTIBYTE=0x31, UCHAR=0x32, AUTOINLINE=0x33,
          BOOL=0x34, ALWAYSINLINE=0x37, INLINELEVEL=0x38, DEFER=0x3c)
CG = dict(ALIGN=2, TB=3, PROC=4, ROSTR=5, TOC=6, PROFILE=7, FPC=8, SCHED=9, PEEP=10, PROCSPEC=11, ALTIVEC=12)
PROCS = ("generic", "601", "603", "603e", "604", "604e", "750", "altivec")


class Opts:
    def __init__(self, argv):
        self.fe = bytearray(62)
        self.fe[1] = 13
        self.fe[FE["ENUMINT"]] = self.fe[FE["MAPCR"]] = self.fe[FE["EXC"]] = self.fe[FE["RTTI"]] = self.fe[FE["BOOL"]] = 1
        self.warn = bytearray(14)
        self.warn[1] = 5
        self.go = bytearray(12)
        self.go[1] = 1
        self.go[2] = 2
        self.cg = bytearray(20)
        self.cg[1] = 5
        self.cg[CG["ALIGN"]] = 2
        self.cg[CG["TOC"]] = self.cg[CG["FPC"]] = self.cg[CG["PEEP"]] = self.cg[CG["SCHED"]] = 1
        self.user, self.source, self.output, self.precompile, self.nosys = [], None, None, None, False
        self.prefix_lines, self.sys_dirs = [], []
        self.verbose = self.preprocess = False
        self.sym = False
        self.nowarn = False
        self.parse(argv)

    def onoff(self, v):
        if v.lower() not in ("on", "off"):
            raise SystemExit("expected on|off, not " + v)
        return int(v.lower() == "on")

    def opt(self, list_):
        for t in list_.split(","):
            t = t.lower()
            if t in ("off", "none"):
                self.go[2] = self.cg[CG["PEEP"]] = self.cg[CG["SCHED"]] = 0
            elif t == "on":
                self.go[2], self.cg[CG["PEEP"]] = 2, 1
            elif t in ("all", "full"):
                self.go[2], self.go[3], self.cg[CG["PEEP"]], self.cg[CG["SCHED"]] = 4, 0, 1, 1
            elif t.startswith("level=") or t.startswith("l=") or (t[:1] == "l" and t[1:].isdigit()):
                self.go[2] = int(t.split("=")[-1].lstrip("l"))
            elif t in ("peephole", "peep"):
                self.cg[CG["PEEP"]] = 1
            elif t in ("nopeephole", "nopeep"):
                self.cg[CG["PEEP"]] = 0
            elif t == "schedule":
                self.cg[CG["SCHED"]] = 1
            elif t == "noschedule":
                self.cg[CG["SCHED"]] = 0
            elif t in ("space", "size", "nospeed"):
                self.go[3] = 1
            elif t in ("speed", "nospace", "nosize"):
                self.go[3] = 0
            else:
                raise SystemExit("unsupported -opt keyword " + t)

    def parse(self, argv):
        i = 0

        def nxt():
            nonlocal i
            i += 1
            return argv[i]
        fe, cg = self.fe, self.cg
        while i < len(argv):
            a = argv[i]
            if not a.startswith("-"):
                if self.source:
                    raise SystemExit("one source per run: " + a)
                self.source = os.path.abspath(a)
                i += 1
                continue
            a = a[1:]
            if a in ("nolink", "nodefaults", "nostdinc", "stderr"):
                pass
            elif a == "nosyspath":
                self.nosys = True
            elif a in ("i", "I"):
                self.user.append(os.path.abspath(nxt()))
            elif a == "o":
                self.output = os.path.abspath(nxt())
            elif a == "precompile":
                self.precompile = os.path.abspath(nxt())
            elif a == "prefix":
                with open(nxt(), "rb") as f:
                    d = f.read()
                if not d.startswith(PFX_MAGIC):
                    raise SystemExit("-prefix: not a file from this tool's -precompile")
                st = json.loads(d[len(PFX_MAGIC):].decode("latin1"))
                self.sys_dirs = st["dirs"]
                self.prefix_lines.insert(0, st["text"])
            elif a == "opt":
                self.opt(nxt())
            elif a == "enum":
                fe[FE["ENUMINT"]] = int(nxt().lower() == "int")
            elif a == "char":
                fe[FE["UCHAR"]] = int(nxt().lower() == "unsigned")
            elif a == "align":
                cg[CG["ALIGN"]] = {"power": 2, "powerpc": 2, "mac68k": 0, "mac68k4byte": 1}[nxt().lower()]
            elif a == "proc":
                cg[CG["PROC"]] = PROCS.index(nxt().lower())
            elif a == "tb":
                cg[CG["TB"]] = {"off": 0, "on": 1, "outofline": 2}[nxt().lower()]
            elif a in ("fp_contract", "maf"):
                cg[CG["FPC"]] = self.onoff(nxt())
            elif a == "toc_data":
                cg[CG["TOC"]] = self.onoff(nxt())
            elif a in ("strings", "str"):
                for t in nxt().lower().split(","):
                    if t == "reuse":
                        fe[FE["NOREUSE"]] = 0
                    elif t in ("dontreuse", "noreuse"):
                        fe[FE["NOREUSE"]] = 1
                    elif t == "pool":
                        fe[FE["POOL"]] = 1
                    elif t == "nopool":
                        fe[FE["POOL"]] = 0
                    elif t in ("readonly", "ro"):
                        cg[CG["ROSTR"]] = 1
                    elif t in ("noreadonly", "noro"):
                        cg[CG["ROSTR"]] = 0
                    else:
                        raise SystemExit("unsupported -strings " + t)
            elif a == "inline":
                for t in nxt().lower().split(","):
                    if t in ("on", "all", "smart"):
                        fe[FE["NOINLINE"]] = 0
                    elif t in ("off", "none"):
                        fe[FE["NOINLINE"]] = 1
                    elif t == "auto":
                        fe[FE["AUTOINLINE"]] = 1
                    elif t == "noauto":
                        fe[FE["AUTOINLINE"]] = 0
                    elif t == "deferred":
                        fe[FE["DEFER"]] = 1
                    elif t.startswith("level="):
                        struct.pack_into(">H", fe, FE["INLINELEVEL"], int(t[6:]))
                    else:
                        raise SystemExit("unsupported -inline " + t)
            elif a == "bool":
                fe[FE["BOOL"]] = self.onoff(nxt())
            elif a == "Cpp_exceptions":
                fe[FE["EXC"]] = self.onoff(nxt())
            elif a == "RTTI":
                fe[FE["RTTI"]] = self.onoff(nxt())
            elif a == "relax_pointers":
                fe[FE["MPWPTR"]] = 1
            elif a == "mapcr":
                fe[FE["MAPCR"]] = 1
            elif a == "nomapcr":
                fe[FE["MAPCR"]] = 0
            elif a == "ansi":
                v = nxt().lower()
                fe[FE["ONLYSTD"]], fe[FE["ENUMINT"]], fe[FE["ANSISTRICT"]] = {"off": (0, 0, 0), "on": (1, 0, 1), "relaxed": (1, 0, 1), "strict": (1, 1, 1)}[v]
            elif a in ("dialect", "lang"):
                fe[FE["CPLUSPLUS"]] = int(nxt().lower() in ("c++", "cplus", "cplusplus"))
            elif a in ("msgstyle", "maxerrors", "maxwarnings"):
                nxt()
            elif a == "D" or (a[0] == "D" and len(a) > 1):
                v = a[1:] if len(a) > 1 else nxt()
                k, eq, val = v.partition("=")
                self.prefix_lines.append("#define %s %s\r" % (k, val if eq else "1"))
            elif a == "U" or (a[0] == "U" and len(a) > 1):
                self.prefix_lines.append("#undef %s\r" % (a[1:] if len(a) > 1 else nxt()))
            elif a == "pragma":      # NAME=VALUE: a #pragma line in the prefix text, what MWCPPC.exe's -pragma does; the optimizer's opt_ switches have no panel field
                self.prefix_lines.append("#pragma %s\r" % nxt().replace("=", " "))
            elif a == "w":
                self.nowarn = nxt().lower() in ("off", "nocmdline")
            elif a == "sym":     # the IDE's debug column, MWCPPC.exe's -sym: the host's answer to CWIsGeneratingDebugInfo
                self.sym = nxt().lower() != "off"
            elif a == "emuverbose":
                self.verbose = True
            elif a == "E":
                self.preprocess = True
            else:
                raise SystemExit("unsupported option -" + a)
            i += 1


# ---------------------------------------------------------------- the emulated machine

class Host:
    def __init__(self, plugdir, o):
        self.o = o
        with open(os.path.join(plugdir, "MW C++ PPC"), "rb") as f:
            self.pef = Pef(f.read())
        with open(os.path.join(plugdir, "MW C++ PPC.rsrc"), "rb") as f:
            self.rsrc = f.read()
        self.licensed = []
        lic = os.path.join(plugdir, "license.dat")
        if os.path.exists(lic):
            with open(lic, "rb") as f:
                for line in f.read().replace(b"\r", b"\n").split(b"\n"):
                    w = line.split()
                    if len(w) > 1 and w[0] in (b"FEATURE", b"INCREMENT"):
                        self.licensed.append(w[1].decode("latin1"))
        self.uc = Uc(UC_ARCH_PPC, UC_MODE_32 | UC_MODE_BIG_ENDIAN)
        self.uc.mem_map(STUB, STUB_SIZE)
        self.uc.mem_map(BASE, RAM)
        self.errors = self.warnings = 0
        self.stored = False
        self.request = 0
        self.dirs = []
        self.refs = {}
        self.included = set()
        self.heap_top = 0
        self.free = {}
        self.sizes = {}
        self.ticks = 0
        self.failure = None
        self.prefix_ptr = None
        self.load()

    # memory
    def r32(self, a):
        return struct.unpack(">I", self.uc.mem_read(a, 4))[0]

    def w32(self, a, v):
        self.uc.mem_write(a, struct.pack(">I", v & 0xffffffff))

    def w16(self, a, v):
        self.uc.mem_write(a, struct.pack(">H", v & 0xffff))

    def r16s(self, a):
        return struct.unpack(">h", self.uc.mem_read(a, 2))[0]

    def w8(self, a, v):
        self.uc.mem_write(a, bytes([v & 0xff]))

    def cstr(self, a):
        out = bytearray()
        while a:
            c = bytes(self.uc.mem_read(a, 256))
            n = c.find(b"\0")
            if n >= 0:
                out += c[:n]
                break
            out += c
            a += 256
        return out.decode("mac_roman")

    def malloc(self, n):
        n = (max(n, 8) + 15) & ~15
        lst = self.free.get(n)
        if lst:
            p = lst.pop()
            self.uc.mem_write(p, bytes(n))
        else:
            p = self.heap_top
            self.heap_top += n
            if self.heap_top > self.stack_low:
                raise CompileFailed("out of emulated memory")
        self.sizes[p] = n
        return p

    def mfree(self, p):
        n = self.sizes.pop(p, None)
        if n:
            self.free.setdefault(n, []).append(p)

    # handles: a Handle points at {ptr, size, cap}
    def hnew(self, size):
        h = self.malloc(12)
        cap = max(size, 16)
        self.w32(h, self.malloc(cap))
        self.w32(h + 4, size)
        self.w32(h + 8, cap)
        return h

    def hsize(self, h):
        return self.r32(h + 4)

    def hresize(self, h, size):
        cap = self.r32(h + 8)
        if size > cap:
            old, olds = self.r32(h), self.r32(h + 4)
            ncap = max(size, cap * 2 if cap < 0x100000 else cap + cap // 2)
            p = self.malloc(ncap)
            self.uc.mem_write(p, bytes(self.uc.mem_read(old, olds)))
            self.mfree(old)
            self.w32(h, p)
            self.w32(h + 8, ncap)
        self.w32(h + 4, size)
        return 1

    def hfree(self, h):
        if h:
            self.mfree(self.r32(h))
            self.mfree(h)

    # machine
    def load(self):
        p, uc = self.pef, self.uc
        bases, cur = {}, BASE
        images = {}
        for i, s in enumerate(p.secs):
            if s["kind"] in (0, 1, 2, 3):
                images[i] = p.image(s)
                bases[i] = cur
                cur = (cur + len(images[i]) + 0xfff) & ~0xfff
        names = p.imports()
        self.imp_names = names
        tv = STUB + 0x8000
        imp = []
        for i, n in enumerate(names):
            uc.mem_write(STUB + 4 * i, struct.pack(">I", 0x4e800020))   # blr, after the hook has run
            uc.mem_write(tv + 8 * i, struct.pack(">II", STUB + 4 * i, 0))
            imp.append(tv + 8 * i)
        uc.mem_write(RET, struct.pack(">I", 0x4e800020))
        p.relocate(images, bases, imp)
        for i, img in images.items():
            uc.mem_write(bases[i], bytes(img))
        self.bases = bases
        self.heap_top = (cur + 0xfff) & ~0xfff
        self.stack_top = BASE + RAM - 0x1000
        self.stack_low = self.stack_top - 0x800000
        uc.hook_add(UC_HOOK_CODE, self.on_import, begin=STUB, end=STUB + 4 * len(names))
        uc.hook_add(UC_HOOK_MEM_INVALID, self.on_bad)
        msr = uc.reg_read(UC_PPC_REG_MSR)
        uc.reg_write(UC_PPC_REG_MSR, msr | 0x2000)      # MSR[FP]
        self.main_tv = bases[p.main_s] + p.main_o
        self.init_tv = bases[p.init_s] + p.init_o if p.init_s >= 0 else None
        self.ctx = self.malloc(1024)

    def on_bad(self, uc, access, addr, size, value, data):
        pc = uc.reg_read(UC_PPC_REG_PC)
        self.failure = "bad memory access 0x%08x at pc 0x%08x (code+0x%x) lr 0x%08x" % (addr, pc, pc - BASE, uc.reg_read(UC_PPC_REG_LR))
        return False

    def on_import(self, uc, addr, size, data):
        i = (addr - STUB) // 4
        name = self.imp_names[i]
        fn = getattr(self, "f_" + name, None)
        args = [uc.reg_read(REG(3 + k)) for k in range(8)]
        try:
            if fn is None:
                raise CompileFailed("import %s not implemented, args %s, lr 0x%08x" % (name, [hex(a) for a in args[:4]], uc.reg_read(UC_PPC_REG_LR)))
            r = fn(*args)
            if self.o.verbose:
                print("[emu] %s(%s) = %s, lr 0x%x" % (name, ", ".join(hex(a) for a in args[:4]), r, uc.reg_read(UC_PPC_REG_LR) - BASE), file=sys.stderr)
            if r is not None:
                uc.reg_write(REG(3), r & 0xffffffff)
        except Exception as e:     # an exception must not cross Unicorn's C frames
            self.failure = "%s: %s" % (name, e)
            uc.emu_stop()

    def call(self, tv, *args):
        uc = self.uc
        code, toc = self.r32(tv), self.r32(tv + 4)
        uc.reg_write(REG(1), self.stack_top - 0x100)
        uc.reg_write(REG(2), toc)
        for k, a in enumerate(args):
            uc.reg_write(REG(3 + k), a)
        uc.reg_write(UC_PPC_REG_LR, RET)
        try:
            uc.emu_start(code, RET)
        except UcError as e:
            raise CompileFailed("%s; %s; pc 0x%08x" % (e, self.failure, uc.reg_read(UC_PPC_REG_PC)))
        if self.failure:
            raise CompileFailed(self.failure)
        v = uc.reg_read(REG(3)) & 0xffff
        return v - 0x10000 if v & 0x8000 else v

    def run(self):
        if self.init_tv:
            block = self.malloc(64)
            self.call(self.init_tv, block)
        self.request = -2
        r = self.call(self.main_tv, self.ctx)
        if r:
            raise CompileFailed("plugin initialisation returned %d" % r)
        self.request = 0
        r = self.call(self.main_tv, self.ctx)
        self.request = -1
        self.call(self.main_tv, self.ctx)
        return r

    # paths and FSSpecs: parID = 1 + index into self.dirs
    def dir_index(self, d):
        k = d.lower()
        for i, x in enumerate(self.dirs):
            if x.lower() == k:
                return i + 1
        self.dirs.append(d)
        return len(self.dirs)

    def spec_from_path(self, a, path):
        d, n = os.path.split(os.path.abspath(path))
        b = n.encode("mac_roman", "replace")[:63]
        self.uc.mem_write(a, struct.pack(">hI", -1, self.dir_index(d)) + bytes([len(b)]) + b + bytes(63 - len(b)))

    def path_from_spec(self, a):
        raw = bytes(self.uc.mem_read(a, FSSPEC))
        par = struct.unpack_from(">I", raw, 2)[0]
        name = raw[7:7 + raw[6]].decode("mac_roman")
        if 1 <= par <= len(self.dirs):
            return os.path.join(self.dirs[par - 1], name)
        return name

    def text_for(self, data):
        t = data.replace(b"\r\n", b"\r").replace(b"\n", b"\r")
        p = self.malloc(len(t) + 1)
        self.uc.mem_write(p, t + b"\0")
        return p, len(t)

    def load_text(self, path):
        with open(path, "rb") as f:
            return self.text_for(f.read())

    # ------------------------------------------------------------ CarbonLib
    def f_NewPtr(self, n, *a):
        return self.malloc(n)

    def f_DisposePtr(self, p, *a):
        self.mfree(p)
        return 0

    def f_TickCount(self, *a):
        self.ticks += 1
        return self.ticks

    def f_GetDateTime(self, p, *a):
        self.w32(p, 0xb6000000)
        return 0

    def f_ReadLocation(self, p, *a):
        self.uc.mem_write(p, bytes(12))
        return 0

    def f_Gestalt(self, sel, resp, *a):
        self.w32(resp, 0)
        return -5551

    def f_feclearexcept(self, *a):
        return 0

    def f_fetestexcept(self, *a):
        return 0

    def f_dec2num(self, p, *a):
        raw = bytes(self.uc.mem_read(p, 40))
        sgn, exp = raw[0], struct.unpack_from(">h", raw, 2)[0]
        digits = raw[5:5 + raw[4]].decode("latin1")
        if digits[:1] in ("I", "i"):
            v = float("inf")
        elif digits[:1] in ("N", "n"):
            v = float("nan")
        else:
            v = float("%se%d" % (digits or "0", exp))
        if sgn:
            v = -v
        self.uc.reg_write(UC_PPC_REG_FPR0 + 1, struct.unpack(">Q", struct.pack(">d", v))[0])
        return None

    def f_num2dec(self, form, *a):
        # PowerOpen: f1 holds the double and shadows r4, r5, so the decimal* is in r6
        dst = a[2]
        v = struct.unpack(">d", struct.pack(">Q", self.uc.reg_read(UC_PPC_REG_FPR0 + 1)))[0]
        raw = bytes(self.uc.mem_read(form, 4))
        style, ndig = raw[0], struct.unpack_from(">h", raw, 2)[0]
        sgn = 1 if str(v).startswith("-") else 0
        v = abs(v)
        if v != v:
            exp, text = 0, "N"
        elif v == float("inf"):
            exp, text = 0, "I"
        elif v == 0:
            exp, text = 0, "0"
        elif style == 0:
            n = max(ndig, 1)
            m, e = ("%.*e" % (n - 1, v)).split("e")
            text, exp = m.replace(".", ""), int(e) - (n - 1)
        else:
            s = "%.*f" % (max(ndig, 0), v)
            exp = -max(ndig, 0)
            text = s.replace(".", "").lstrip("0") or "0"
        b = text.encode("latin1")[:36]
        self.uc.mem_write(dst, bytes([sgn, 0]) + struct.pack(">h", exp) + bytes([len(b)]) + b)
        return None

    # ------------------------------------------------------------ COS_*
    def f_COS_NewHandle(self, n, *a):
        return self.hnew(n)

    f_COS_NewOSHandle = f_COS_NewHandle

    def f_COS_FreeHandle(self, h, *a):
        self.hfree(h)
        return 0

    def f_COS_ResizeHandle(self, h, n, *a):
        return self.hresize(h, n)

    def f_COS_LockHandle(self, *a):
        return 0

    f_COS_LockHandleHi = f_COS_UnlockHandle = f_COS_LockHandle

    def f_COS_GetTicks(self, *a):
        self.ticks += 1
        return self.ticks

    def f_COS_IsMultiByte(self, *a):
        return 0

    def f_COS_GetString(self, buf, sid, idx, *a):
        sid, idx = _s16(sid), _s16(idx)
        self.w8(buf, 0)
        s = str_resource(self.rsrc, sid, idx)
        if s is not None:
            self.uc.mem_write(buf, s + b"\0")
        return 0

    def newref(self, f):
        r = 1
        while r in self.refs:
            r += 1
        self.refs[r] = f
        return r

    def f_COS_FileNew(self, spec, pref, *a):
        try:
            f = open(self.path_from_spec(spec), "w+b")
        except OSError:
            return -43
        self.w16(pref, self.newref(f))
        return 0

    def f_COS_FileOpen(self, spec, pref, *a):
        try:
            f = open(self.path_from_spec(spec), "rb")
        except OSError:
            return -43
        self.w16(pref, self.newref(f))
        return 0

    def f_COS_FileGetType(self, spec, ptype, *a):
        if not os.path.isfile(self.path_from_spec(spec)):
            return -43
        self.w32(ptype, 0x54455854)
        return 0

    def f_COS_FileGetSize(self, ref, p, *a):
        f = self.refs.get(_s16(ref))
        if not f:
            return -51
        at = f.tell()
        f.seek(0, 2)
        self.w32(p, f.tell())
        f.seek(at)
        return 0

    def f_COS_FileRead(self, ref, buf, n, *a):
        f = self.refs.get(_s16(ref))
        if not f:
            return -51
        d = f.read(n)
        self.uc.mem_write(buf, d)
        return 0 if len(d) == n else -39

    def f_COS_FileWrite(self, ref, buf, n, *a):
        f = self.refs.get(_s16(ref))
        if not f:
            return -51
        f.write(bytes(self.uc.mem_read(buf, n)))
        return 0

    def f_COS_FileGetPos(self, ref, p, *a):
        f = self.refs.get(_s16(ref))
        if not f:
            return -51
        self.w32(p, f.tell())
        return 0

    def f_COS_FileSetPos(self, ref, pos, *a):
        f = self.refs.get(_s16(ref))
        if not f:
            return -51
        f.seek(pos)
        return 0

    def f_COS_FileClose(self, ref, *a):
        f = self.refs.pop(_s16(ref), None)
        if not f:
            return -51
        f.close()
        return 0

    def f_COS_FileSetFSSpec(self, spec, ppath, *a):
        n = bytes(self.uc.mem_read(ppath, 1))[0]
        p = bytes(self.uc.mem_read(ppath + 1, n)).decode("mac_roman")
        if not os.path.isabs(p):
            d = self.path_from_spec(spec)
            p = os.path.join(os.path.dirname(d), p)
        self.spec_from_path(spec, p)
        return 0

    def f_COS_FileGetFSSpecInfo(self, spec, pv, pd, pn, *a):
        raw = bytes(self.uc.mem_read(spec, FSSPEC))
        if pv:
            self.uc.mem_write(pv, raw[:2])
        if pd:
            self.uc.mem_write(pd, raw[2:6])
        if pn:
            self.uc.mem_write(pn, raw[6:7 + raw[6]])
        return 0

    def f_COS_FileGetPathName(self, buf, spec, pdate, *a):
        if pdate:
            self.w32(pdate, 0)
        self.uc.mem_write(buf, self.path_from_spec(spec).encode("mac_roman", "replace") + b"\0")
        return 0

    # ------------------------------------------------------------ CW*
    def f_CWGetPluginRequest(self, ctx, p, *a):
        self.w32(p, self.request)
        return 0

    def f_CWDonePluginRequest(self, ctx, res, *a):
        return res

    def f_CWGetAPIVersion(self, ctx, p, *a):
        self.w32(p, 9)
        return 0

    def f_CWCheckoutLicense(self, ctx, pfeature, pversion, flags, _r, pcookie, *a):
        # as the IDE does: a feature is granted only if the install's own license.dat lists it. Its signature is not verified.
        feature = self.cstr(pfeature)
        granted = feature in self.licensed
        self.w32(pcookie, 1 + self.licensed.index(feature) if granted else 0)
        if self.o.verbose:
            print("[emu] licence", feature, "granted" if granted else "refused", file=sys.stderr)
        return 0 if granted else 2      # cwErrRequestFailed

    def f_CWCheckinLicense(self, *a):
        return 0

    def f_CWGetProjectFile(self, ctx, spec, *a):
        self.spec_from_path(spec, os.path.join(os.getcwd(), "emu.mcp"))
        return 0

    def f_CWGetProjectFileCount(self, ctx, p, *a):
        self.w32(p, 1)
        return 0

    def f_CWGetMainFileNumber(self, ctx, p, *a):
        self.w32(p, 0)
        return 0

    def f_CWGetMainFileID(self, ctx, p, *a):
        self.w16(p, 0)
        return 0

    def f_CWGetMainFileSpec(self, ctx, spec, *a):
        self.spec_from_path(spec, self.o.source)
        return 0

    def f_CWGetMainFileText(self, ctx, ptext, plen, *a):
        try:
            t, n = self.load_text(self.o.source)
        except OSError:
            return 8
        self.included.add(self.o.source.lower())
        self.w32(ptext, t)
        self.w32(plen, n)
        return 0

    def f_CWIsPrecompiling(self, ctx, p, *a):
        self.w8(p, 1 if self.o.precompile else 0)
        return 0

    def f_CWIsAutoPrecompiling(self, ctx, p, *a):
        self.w8(p, 0)
        return 0

    def f_CWIsPreprocessing(self, ctx, p, *a):
        self.w8(p, 1 if self.o.preprocess else 0)
        return 0

    def f_CWIsGeneratingDebugInfo(self, ctx, p, *a):
        self.w8(p, 1 if self.o.sym else 0)
        return 0

    f_CWIsCachingPrecompiledHeaders = f_CWIsAutoPrecompiling

    def f_CWGetBrowseOptions(self, ctx, p, *a):
        self.uc.mem_write(p, bytes(16))
        return 0

    def f_CWGetTargetInfo(self, ctx, p, *a):
        self.uc.mem_write(p, bytes(240))
        self.w16(p, 1)
        self.uc.mem_write(p + 216, b"ppc mac ")
        return 0

    def f_CWGetTargetName(self, ctx, buf, n, *a):
        self.uc.mem_write(buf, b"emu\0")
        return 0

    def panel(self, name):
        o = self.o
        return {"C/C++ Compiler": o.fe, "C/C++ Warnings": o.warn, "PPC Global Optimizer": o.go, "PPC CodeGen": o.cg}.get(name)

    def f_CWSecretGetNamedPreferences(self, ctx, pname, pout, *a):
        name = self.cstr(pname)
        d = self.panel(name)
        if self.o.verbose:
            print("[emu] prefs", name, "(unknown)" if d is None else "", file=sys.stderr)
        if d is None:
            return 2
        h = self.hnew(len(d))
        self.uc.mem_write(self.r32(h), bytes(d))
        self.w32(pout, h)
        return 0

    def f_CWSecretAttachHandle(self, ctx, h, pout, *a):
        self.w32(pout, h)
        return 0

    def f_CWAllocMemHandle(self, ctx, n, temp, pout, *a):
        self.w32(pout, self.hnew(n))
        return 0

    def f_CWFreeMemHandle(self, ctx, h, *a):
        self.hfree(h)
        return 0

    def f_CWLockMemHandle(self, ctx, h, hi, pp, *a):
        if not h:
            return 3
        self.w32(pp, self.r32(h))
        return 0

    def f_CWUnlockMemHandle(self, *a):
        return 0

    def find_include(self, name, full):
        o = self.o
        nm = name.replace("/", os.sep).replace(":", os.sep) if not (len(name) > 1 and name[1] == ":") else name
        if os.path.isabs(nm):
            return nm if os.path.isfile(nm) else None
        # -nosyspath: the -i list serves both forms. <x> tries the prefix's directories first, so Apple's Dialogs.h, reached
        # from Apple's own headers in the prefix, is not taken for the tree's dialogs.h on a case-blind path.
        for d in (o.sys_dirs if not full else []) + o.user:
            p = os.path.join(d, nm)
            if os.path.isfile(p):
                return os.path.normpath(p)
        return None

    def f_CWFindAndLoadFile(self, ctx, pname, fi, *a):
        name = self.cstr(pname)
        raw = bytes(self.uc.mem_read(fi, 8))
        full, suppress = raw[0], raw[6]
        if self.o.verbose:
            print("[emu] find", name, "full", full, "dep", struct.unpack_from(">i", raw, 2)[0], "deptype", raw[1], file=sys.stderr)
        if name == "emu prefix":
            t = "".join(self.o.prefix_lines).encode("latin1")
            p, n = self.text_for(t)
            self.w32(fi + 8, p)
            self.w32(fi + 12, n)
            self.w16(fi + 16, 1)
            self.w16(fi + 18, 1)
            self.spec_from_path(fi + 20, os.path.join(os.getcwd(), "emu prefix"))
            self.w8(fi + 90, 0)
            self.w8(fi + 91, 0)
            return 0
        path = self.find_include(name, full)
        if self.o.verbose:
            print("[emu] include", name, "->", path, file=sys.stderr)
        if not path:
            return 8
        self.spec_from_path(fi + 20, path)
        k = path.lower()
        self.w8(fi + 90, 1 if k in self.included else 0)
        self.included.add(k)
        if suppress:
            self.w32(fi + 8, 0)
            self.w32(fi + 12, os.path.getsize(path))
        else:
            t, n = self.load_text(path)
            self.w32(fi + 8, t)
            self.w32(fi + 12, n)
        self.w16(fi + 16, 1)
        self.w16(fi + 18, len(self.included) + 1)
        self.w8(fi + 91, 0)
        return 0

    def f_CWGetFileText(self, ctx, spec, ptext, plen, ptype, *a):
        try:
            t, n = self.load_text(self.path_from_spec(spec))
        except OSError:
            return 8
        self.w32(ptext, t)
        self.w32(plen, n)
        self.w16(ptype, 1)
        return 0

    def f_CWReleaseFileText(self, *a):
        return 0

    def f_CWReportMessage(self, ctx, ref, l1, l2, level, *a):
        level = _s16(level)
        if level == 2:
            self.errors += 1
        elif level == 1:
            self.warnings += 1
        if level == 1 and self.o.nowarn:
            return 0
        kind = {2: "Error", 1: "Warning"}.get(level, "Note")
        msg = self.cstr(l1).replace("\r", " ") if l1 else ""
        if ref:
            raw = bytes(self.uc.mem_read(ref, FSSPEC + 4))
            name = raw[7:7 + raw[6]].decode("mac_roman")
            line = struct.unpack_from(">I", raw, FSSPEC)[0]
            print("%s:%d: %s" % (name, line, msg) if level == 2 else "%s:%d: warning: %s" % (name, line, msg), flush=True)
        else:
            print("%s: %s" % (kind, msg), flush=True)
        return 0

    def f_CWShowStatus(self, *a):
        return 0

    f_CWUserBreak = f_CWDisplayLines = f_CWSetModDate = f_CWShowStatus

    def f_CWGetPrecompiledHeaderSpec(self, ctx, spec, target, *a):
        self.spec_from_path(spec, self.o.precompile or "emu.pch")
        return 0

    def f_CWCachePrecompiledHeader(self, *a):
        return 0

    def f_CWCreateNewTextDocument(self, ctx, doc, *a):
        h = self.r32(doc + 4)
        d = bytes(self.uc.mem_read(self.r32(h), self.hsize(h))) if h else b""
        d = d.replace(b"\r", b"\n")
        if self.o.output:
            with open(self.o.output, "wb") as f:
                f.write(d)
        else:
            sys.stdout.buffer.write(d)
        self.stored = True
        return 0

    def f_CWStoreObjectData(self, ctx, which, od, *a):
        h = self.r32(od)
        if h:
            obj = bytes(self.uc.mem_read(self.r32(h), self.hsize(h)))
            write_mwob(self.o.output, self.o.source, obj)
        self.stored = True
        return 0


def _s16(v):
    v &= 0xffff
    return v - 0x10000 if v & 0x8000 else v


def str_resource(fork, sid, index):
    data, mapo = struct.unpack_from(">II", fork, 0)
    tl = mapo + struct.unpack_from(">H", fork, mapo + 24)[0]
    for t in range(struct.unpack_from(">H", fork, tl)[0] + 1):
        ty, cnt, ref = struct.unpack_from(">4sHH", fork, tl + 2 + 8 * t)
        if ty != b"STR#":
            continue
        for k in range(cnt + 1):
            r = tl + ref + 12 * k
            if struct.unpack_from(">h", fork, r)[0] != sid:
                continue
            off = struct.unpack_from(">I", fork, r + 4)[0] & 0xffffff
            d = data + off + 4
            count = struct.unpack_from(">H", fork, d)[0]
            if index < 1 or index > count:
                return None
            s = d + 2
            for _ in range(index - 1):
                s += 1 + fork[s]
            return fork[s + 1:s + 1 + fork[s]]
    return None


def write_mwob(path, source, obj):
    """A one-member MWOB library round the plugin's POWR record, as MWCPPC.exe writes it (mwcppc22 host.c)."""
    base = os.path.basename(source).encode("mac_roman") + b"\0"
    full = source.encode("mac_roman", "replace") + b"\0"
    doff = (0x30 + len(base) + len(full) + 3) & ~3
    code = udata = 0
    if len(obj) >= 40:
        code, ud, id_ = struct.unpack_from(">III", obj, 28)
        udata = ud + id_
    mdate = int(os.path.getmtime(source)) + 2082758400   # as MWCPPC.exe computes Mac seconds
    hdr = b"MWOBPPC " + struct.pack(">IIIIIIIIII", 0, 1, code, udata, 1, mdate & 0xffffffff, 0x30, 0x30 + len(base), doff, len(obj))
    with open(path, "wb") as f:
        f.write(hdr + base + full + bytes(doff - 0x30 - len(base) - len(full)) + obj)


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--plugin" not in argv:
        raise SystemExit("--plugin DIR is required")
    i = argv.index("--plugin")
    plugdir = argv[i + 1]
    del argv[i:i + 2]
    o = Opts(argv)
    if not o.source:
        raise SystemExit("no source file")
    if not o.output and not o.preprocess and not o.precompile:
        o.output = os.path.splitext(os.path.basename(o.source))[0] + ".o"
    if o.prefix_lines and not o.precompile:
        b = b"emu prefix"
        o.fe[FE["PREFIX"]] = len(b)
        o.fe[FE["PREFIX"] + 1:FE["PREFIX"] + 1 + len(b)] = b
    if o.precompile:    # recorded and replayed as text per compile: the plugin's real PCH read back wrong (tooling-reference)
        with open(o.source, "rb") as f:
            text = f.read().decode("latin1").replace("\r\n", "\r").replace("\n", "\r")
        with open(o.precompile, "wb") as f:
            f.write(PFX_MAGIC + json.dumps(dict(text=text, dirs=o.user)).encode("latin1"))
        return 0
    t0 = time.time()
    h = Host(plugdir, o)
    try:
        r = h.run()
    except CompileFailed as e:
        print("mwppcemu:", e, file=sys.stderr)
        return 2
    if o.verbose:
        print("[emu] result %d, %d errors, %d warnings, %.1f s" % (r, h.errors, h.warnings, time.time() - t0), file=sys.stderr)
    if r or h.errors:
        if o.output and os.path.exists(o.output) and not o.preprocess:
            os.remove(o.output)
        return 1
    if not h.stored and not o.precompile:
        print("mwppcemu: plugin stored no object", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
