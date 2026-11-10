"""COFF objects and archives: reading symbol tables, renaming symbols, writing an archive and a
symbols-only object. Section bytes and relocations are never touched."""
import struct
from pathlib import Path

EXTERNAL = 2


def symbols(b):
    """[(record index, name, section number, storage class, value)] of a COFF object."""
    if len(b) < 20:
        return []
    nsec, _ts, symp, nsym, _opt = struct.unpack_from("<HIIIH", b, 2)
    strtab = symp + nsym * 18
    out, i = [], 0
    while i < nsym:
        e = b[symp + i * 18:symp + i * 18 + 18]
        if e[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", e, 4)[0]
            name = b[strtab + off:b.index(b"\0", strtab + off)].decode("latin-1")
        else:
            name = e[:8].rstrip(b"\0").decode("latin-1")
        value, sec, _t, sc, naux = struct.unpack_from("<IhHBB", e, 8)
        out.append((i, name, sec, sc, value))
        i += 1 + naux
    return out


def defined(b):
    """External names an object defines (a COMMON counts)."""
    return {n for _i, n, sec, sc, v in symbols(b) if sc == EXTERNAL and (sec > 0 or (sec == 0 and v > 0))}


def undefined(b):
    return {n for _i, n, sec, sc, v in symbols(b) if sc == EXTERNAL and sec == 0 and v == 0}


def code_and_data(b):
    """The external definitions in section order: [(section, name, is code)]."""
    nsec, _ts, _symp, _nsym, opt = struct.unpack_from("<HIIIH", b, 2)
    chars = [struct.unpack_from("<I", b, 20 + opt + 40 * k + 36)[0] for k in range(nsec)]
    defs = [(sec, n, bool(chars[sec - 1] & 0x20)) for _i, n, sec, sc, _v in symbols(b) if sc == EXTERNAL and sec > 0]
    return sorted(defs)


def pull_symbol(b, avoid=None):
    """The name that pulls this member out of an archive: its first code symbol, else its first data symbol."""
    defs = [d for d in code_and_data(b) if not (avoid and d[1].startswith(avoid))]
    code = [n for _s, n, c in defs if c]
    return (code or [n for _s, n, _c in defs] or [None])[0]


def rename(b, renames):
    """B with every external symbol record named in RENAMES renamed; the string table grows."""
    _nsec, _ts, symp, nsym, _opt = struct.unpack_from("<HIIIH", b, 2)
    strtab = symp + nsym * 18
    size = struct.unpack_from("<I", b, strtab)[0]
    if strtab + size != len(b):
        raise ValueError("string table is not at the end of the object")
    d, extra = bytearray(b), bytearray()
    for i, name, _sec, sc, _v in symbols(b):
        if sc == EXTERNAL and name in renames:
            struct.pack_into("<II", d, symp + i * 18, 0, size + len(extra))
            extra += renames[name].encode("latin-1") + b"\0"
    struct.pack_into("<I", d, strtab, size + len(extra))
    return bytes(d + extra)


def members(path):
    """[(member file name, bytes)] of a COFF archive, long names resolved, directories dropped."""
    d = Path(path).read_bytes()
    if d[:8] != b"!<arch>\n":
        raise ValueError("not an archive: %s" % path)
    p, longnames, out = 8, b"", []
    while p + 60 <= len(d):
        h = d[p:p + 60]
        name = h[:16].decode("latin-1").strip()
        size = int(h[48:58])
        body = d[p + 60:p + 60 + size]
        if name == "//":
            longnames = body
        elif name != "/":
            if name.startswith("/") and name[1:].isdigit():
                k = int(name[1:])
                name = longnames[k:longnames.find(b"\0", k)].decode("latin-1")
            out.append((name.rstrip("/").replace("\\", "/").split("/")[-1], body))
        p += 60 + size + (size & 1)
    return out


def write_archive(path, items):
    """An MS COFF archive (first and second linker members, long-name table) of ITEMS [(name, bytes)]."""
    index = []
    for k, (_n, b) in enumerate(items):
        index += [(n, k) for n in sorted_defs(b)]
    longnames, hdrname = bytearray(), []
    for n, _b in items:
        if len(n) <= 15:
            hdrname.append((n + "/").encode("latin-1"))
        else:
            hdrname.append(("/%d" % len(longnames)).encode("latin-1"))
            longnames += n.encode("latin-1") + b"\0"

    def header(name, size):
        return (name.ljust(16) + b"0".ljust(12) + b"".ljust(6) + b"".ljust(6) + b"0".ljust(8) +
                str(size).encode().ljust(10) + b"`\n")

    names1 = b"".join(s.encode("latin-1") + b"\0" for s, _k in index)
    size1 = 4 + 4 * len(index) + len(names1)
    srt = sorted(index, key=lambda x: x[0].encode("latin-1"))
    names2 = b"".join(s.encode("latin-1") + b"\0" for s, _k in srt)
    size2 = 4 + 4 * len(items) + 4 + 2 * len(srt) + len(names2)
    off = 8 + 60 + size1 + (size1 & 1) + 60 + size2 + (size2 & 1)
    if longnames:
        off += 60 + len(longnames) + (len(longnames) & 1)
    offs = []
    for _n, b in items:
        offs.append(off)
        off += 60 + len(b) + (len(b) & 1)
    out = bytearray(b"!<arch>\n")
    out += header(b"/", size1) + struct.pack(">I", len(index))
    out += b"".join(struct.pack(">I", offs[k]) for _s, k in index) + names1
    out += b"\n" if size1 & 1 else b""
    out += header(b"/", size2) + struct.pack("<I", len(items))
    out += b"".join(struct.pack("<I", o) for o in offs) + struct.pack("<I", len(srt))
    out += b"".join(struct.pack("<H", k + 1) for _s, k in srt) + names2
    out += b"\n" if size2 & 1 else b""
    if longnames:
        out += header(b"//", len(longnames)) + longnames
        out += b"\n" if len(longnames) & 1 else b""
    for (_n, b), h in zip(items, hdrname):
        out += header(h, len(b)) + b
        out += b"\n" if len(b) & 1 else b""
    Path(path).write_bytes(bytes(out))


def sorted_defs(b):
    """An archive index's names for one member: its definitions in section order, then its COMMONs."""
    common = [n for _i, n, sec, sc, v in symbols(b) if sc == EXTERNAL and sec == 0 and v > 0]
    return [n for _s, n, _c in code_and_data(b)] + common


def write_undefined_object(path, names):
    """An object with no sections whose symbol table is NAMES, each an undefined external: the
    first input of a link, it makes the linker pull those names in that order."""
    strtab, symtab = bytearray(b"\0\0\0\0"), bytearray()
    for s in names:
        raw = s.encode("latin-1")
        if len(raw) <= 8:
            nm = raw.ljust(8, b"\0")
        else:
            nm = struct.pack("<II", 0, len(strtab))
            strtab += raw + b"\0"
        symtab += nm + struct.pack("<IhHBB", 0, 0, 0, EXTERNAL, 0)
    struct.pack_into("<I", strtab, 0, len(strtab))
    Path(path).write_bytes(struct.pack("<HHIIIHH", 0x14C, 0, 0, 20, len(names), 0, 0) + bytes(symtab) + bytes(strtab))
