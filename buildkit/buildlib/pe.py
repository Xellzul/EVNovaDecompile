"""PE post-steps of the Windows link: the date pin, the large-address-aware bit, the copied
Authenticode signature, and reading the icon resources out of the user's exe."""
import struct
from pathlib import Path

ORIGINAL_STAMP = 0x449CBBC3            # EV Nova.exe 1.0.10's TimeDateStamp, 2006-06-24 04:12:51 UTC
LARGE_ADDRESS_AWARE = 0x0020


class Pe:
    def __init__(self, d):
        if d[:2] != b"MZ":
            raise ValueError("not a PE file")
        lfanew = struct.unpack_from("<I", d, 0x3C)[0]
        if d[lfanew:lfanew + 4] != b"PE\0\0":
            raise ValueError("not a PE file")
        coff = lfanew + 4
        self.stamp_off = coff + 4
        self.chars_off = coff + 18
        nsec, = struct.unpack_from("<H", d, coff + 2)
        optsize, = struct.unpack_from("<H", d, coff + 16)
        opt = coff + 20
        self.chk_off = opt + 64
        self.sec_off = opt + 96 + 4 * 8                # IMAGE_DIRECTORY_ENTRY_SECURITY (PE32)
        self.sections = []
        for i in range(nsec):
            o = opt + optsize + 40 * i
            name = d[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
            vsize, va, raw, ptr = struct.unpack_from("<IIII", d, o + 8)
            self.sections.append((name, va, vsize, ptr, raw))


def checksum(d, chk_off):
    """The PE CheckSum (the Windows algorithm: 16-bit one's-complement sum plus the file length)."""
    s = 0
    pad = d + (b"\0" if len(d) & 1 else b"")
    for i in range(0, len(pad), 2):
        if i == chk_off or i == chk_off + 2:
            continue
        s += pad[i] | (pad[i + 1] << 8)
        s = (s & 0xFFFF) + (s >> 16)
    return ((s & 0xFFFF) + (s >> 16) & 0xFFFF) + len(d)


def _finish(d, pe):
    if struct.unpack_from("<I", d, pe.chk_off)[0]:   # redo the CheckSum only where the linker wrote one
        struct.pack_into("<I", d, pe.chk_off, checksum(bytes(d), pe.chk_off))
    return bytes(d)


def set_stamp(path, stamp=ORIGINAL_STAMP):
    d = bytearray(Path(path).read_bytes())
    pe = Pe(d)
    struct.pack_into("<I", d, pe.stamp_off, stamp)
    Path(path).write_bytes(_finish(d, pe))


def set_large_address_aware(path):
    d = bytearray(Path(path).read_bytes())
    pe = Pe(d)
    chars = struct.unpack_from("<H", d, pe.chars_off)[0]
    struct.pack_into("<H", d, pe.chars_off, chars | LARGE_ADDRESS_AWARE)
    Path(path).write_bytes(_finish(d, pe))


def copy_signature(path, original):
    """Append the WIN_CERTIFICATE the user's own exe ends with, at the next 8-byte boundary, and
    point the security directory at it. Valid only for an image identical to the original's."""
    o = Path(original).read_bytes()
    off, size = struct.unpack_from("<II", o, Pe(o).sec_off)
    if not size or off + size != len(o):
        raise ValueError("%s has no certificate table at its end" % original)
    d = bytearray(Path(path).read_bytes())
    pe = Pe(d)
    have_off, have_size = struct.unpack_from("<II", d, pe.sec_off)
    if have_size:
        del d[have_off:]
    d += b"\0" * (-len(d) & 7)
    at = len(d)
    d += o[off:off + size]
    struct.pack_into("<II", d, pe.sec_off, at, size)
    Path(path).write_bytes(_finish(d, pe))


def resources(path):
    """[(type, id, bytes)] of a PE's resource directory, in directory order."""
    d = Path(path).read_bytes()
    secs = Pe(d).sections
    rs = [s for s in secs if s[0] == ".rsrc"]
    if not rs:
        return []
    base = rs[0][3]

    def at_rva(rva):
        for _n, va, vs, ptr, raw in secs:
            if va <= rva < va + max(vs, raw):
                return ptr + rva - va
        raise ValueError("resource data outside every section")

    def walk(off, keys):
        o = base + off
        nnamed, nid = struct.unpack_from("<HH", d, o + 12)
        out = []
        for i in range(nnamed + nid):
            nameid, dataoff = struct.unpack_from("<II", d, o + 16 + i * 8)
            if nameid & 0x80000000:
                no = base + (nameid & 0x7FFFFFFF)
                key = d[no + 2:no + 2 + 2 * struct.unpack_from("<H", d, no)[0]].decode("utf-16le")
            else:
                key = nameid
            if dataoff & 0x80000000:
                out += walk(dataoff & 0x7FFFFFFF, keys + [key])
            else:
                rva, size = struct.unpack_from("<II", d, base + dataoff)
                out.append((keys[0], keys[1], d[at_rva(rva):at_rva(rva) + size]))
        return out
    return walk(0, [])
