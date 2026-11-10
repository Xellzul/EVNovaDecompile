"""InstallShield installers, unpacked as the installer would lay the files down, without running it."""
import os
import re
import shutil
import struct
import subprocess
import tempfile


def sfx7(exe, out):
    """An InstallShield 7 setup.exe: its overlay is entries `name NUL path NUL version NUL size NUL` + bytes."""
    d = open(exe, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    sec = pe + 24 + struct.unpack_from("<H", d, pe + 20)[0]
    o = max(sum(struct.unpack_from("<II", d, sec + 40 * i + 16)) for i in range(struct.unpack_from("<H", d, pe + 6)[0]))
    while o < len(d):
        parts = []
        for _ in range(4):
            e = d.index(b"\0", o)
            parts.append(d[o:e].decode("latin-1"))
            o = e + 1
        size = int(parts[3])
        dst = os.path.join(out, *parts[1].split("\\"))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, "wb").write(d[o:o + size])
        o += size


def listing(cab):
    out = subprocess.run(["unshield", "l", cab], check=True, capture_output=True).stdout.decode("latin-1")
    for line in out.splitlines():
        m = re.match(r"^\s*(\d+)\s{2}(.+\\.+)$", line)
        if m:
            yield int(m.group(1)), m.group(2).split("\\")


def find_ci(root, parts):
    """the path under ROOT matching PARTS case-insensitively (Windows: an update replaces a file of any case)"""
    cur = root
    for p in parts:
        hit = next((e for e in os.listdir(cur) if e.lower() == p.lower()), p) if os.path.isdir(cur) else p
        cur = os.path.join(cur, hit)
    return cur


def groups(cab, dest, names, only=None):
    """Lay the files of the file groups NAMES of CAB under DEST (each group's own folder dropped), with the
    names `unshield l` prints: its extraction writes spaces in folder names as underscores. ONLY: a file name."""
    tmp = tempfile.mkdtemp(dir="/var/tmp")
    try:
        for g in names:
            subprocess.run(["unshield", "-R", "-g", g, "-d", tmp, "x", cab], check=True, stdout=subprocess.DEVNULL)
        extracted = {}
        for dp, _, fn in os.walk(tmp):
            for f in fn:
                p = os.path.join(dp, f)
                extracted.setdefault((f, os.path.getsize(p)), []).append(p)
        n = 0
        for size, parts in listing(cab):
            if parts[0] not in names or (only and parts[-1] != only):
                continue
            src = os.path.join(tmp, *[re.sub(r"[ <>]", "_", p) for p in parts[:-1]] + parts[-1:])
            if not os.path.isfile(src):                  # an odd group name: the one file of that name and size
                hits = extracted.get((parts[-1], size), [])
                if len(hits) != 1:
                    raise RuntimeError("cannot place %s" % "\\".join(parts))
                src = hits[0]
            if os.path.getsize(src) != size:
                raise RuntimeError("size of %s" % "\\".join(parts))
            dst = find_ci(dest, parts[1:])
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.move(src, dst)
            n += 1
        return n
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
