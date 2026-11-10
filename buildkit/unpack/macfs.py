"""Classic Mac containers, read without mounting or running anything: HFS and HFS+ volumes, DiskCopy 6 NDIF
images, MacBinary, AppleDouble and Installer VISE archives."""
import re
import struct
import zlib


def u16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def u32(b, o):
    return struct.unpack_from(">I", b, o)[0]


# -- classic HFS: a CD, a Toast, a floppy image ----------------------------------------------------
class Hfs:
    def __init__(self, path):
        self.f = open(path, "rb")
        self.base = self.find_volume()
        mdb = self.read(self.base + 1024, 162)
        self.alblksiz, self.alblst = u32(mdb, 20), u16(mdb, 28)
        self.overflow = {}
        for key, data in self.leaves(self.fork(self.extents(mdb[134:146]), u32(mdb, 130))):
            ftype, fid, start = struct.unpack(">BIH", key[1:8])
            self.overflow.setdefault((fid, ftype), []).append((start, self.extents(data[:12])))
        self.ct = self.fork(self.extents(mdb[150:162]), u32(mdb, 146))

    def read(self, off, n):
        self.f.seek(off)
        return self.f.read(n)

    def find_volume(self):
        for off in (0, 0x800):
            if self.read(off + 1024, 2) == b"BD":
                return off
        for i in range(1, 64):                     # an Apple partition map: the first Apple_HFS partition
            e = self.read(512 * i, 512)
            if e[:2] != b"PM":
                break
            if e[48:80].split(b"\0")[0] == b"Apple_HFS":
                return 512 * u32(e, 8)
        raise ValueError("no HFS volume")

    @staticmethod
    def extents(b):
        return [struct.unpack(">HH", b[4 * i:4 * i + 4]) for i in range(3) if u16(b, 4 * i + 2)]

    def fork(self, ext, length, fid=None, ftype=0):
        ext = list(ext)
        if fid is not None:
            have = sum(c for _, c in ext)
            for start, more in sorted(self.overflow.get((fid, ftype), [])):
                if start >= have:
                    ext += more
                    have += sum(c for _, c in more)
        out = bytearray()
        for s, c in ext:
            out += self.read(self.base + self.alblst * 512 + s * self.alblksiz, c * self.alblksiz)
        if len(out) < length:
            raise ValueError("short fork of file %s" % fid)
        return bytes(out[:length])

    @staticmethod
    def leaves(tree):
        nsize, node = u16(tree, 32), u32(tree, 24)
        while node:
            n = tree[node * nsize:(node + 1) * nsize]
            for r in range(u16(n, 10)):
                off = u16(n, nsize - 2 * (r + 1))
                key = n[off:off + 1 + n[off]]
                d = off + 1 + n[off]
                yield key, n[d + (d & 1):]
            node = u32(n, 0)

    def files(self):
        """(path parts below the volume, catalog record) of every file"""
        dirs, files = {}, []
        for key, data in self.leaves(self.ct):
            parent, name = u32(key, 2), key[7:7 + key[6]].decode("mac_roman")
            if data[0] == 1:
                dirs[u32(data, 6)] = (parent, name)
            elif data[0] == 2:
                files.append((parent, name, data))
        for parent, name, data in files:
            parts = [name]
            while parent in dirs:
                parent, n = dirs[parent]
                parts.append(n)
            yield list(reversed(parts))[1:], data      # [0] was the volume name

    def forks(self, data):
        fid = u32(data, 20)
        return (self.fork(self.extents(data[74:86]), u32(data, 26), fid, 0x00),
                self.fork(self.extents(data[86:98]), u32(data, 36), fid, 0xFF))


# -- HFS+ (bare or inside an HFS wrapper); the first eight extents of a fork only -------------------
class HfsPlus:
    def __init__(self, path):
        self.f = open(path, "rb")
        self.base = 0
        vh = self.read(1024, 512)
        if vh[:2] == b"BD" and u16(vh, 124) == 0x482B:
            self.base = u16(vh, 28) * 512 + u16(vh, 126) * u32(vh, 20)
            vh = self.read(self.base + 1024, 512)
        if vh[:2] not in (b"H+", b"HX"):
            raise ValueError("not an HFS+ volume")
        self.bs = u32(vh, 40)
        self.catalog = self.forkdata(vh[272:352])

    def read(self, off, n):
        self.f.seek(off)
        return self.f.read(n)

    def forkdata(self, fd):
        size = struct.unpack(">Q", fd[:8])[0]
        ext = [struct.unpack(">II", fd[16 + 8 * i:24 + 8 * i]) for i in range(8)]
        if sum(c for _, c in ext) < u32(fd, 12):
            raise ValueError("fork with overflow extents")
        return b"".join(self.read(self.base + s * self.bs, c * self.bs) for s, c in ext if c)[:size]

    def file(self, path):
        """(data fork, resource fork) of the file at PATH, a list of names below the root folder"""
        cat = self.catalog
        nsize, node, recs = u16(cat, 32), u32(cat, 24), {}
        while node:
            n = cat[node * nsize:(node + 1) * nsize]
            for i in range(u16(n, 10)):
                o = u16(n, nsize - 2 * (i + 1))
                name = n[o + 8:o + 8 + 2 * u16(n, o + 6)].decode("utf-16-be")
                recs[(u32(n, o + 2), name)] = n[o + 2 + u16(n, o):]
            node = u32(n, 0)
        cnid = 2
        for i, p in enumerate(path):
            rec = recs[(cnid, p)]
            if i < len(path) - 1:
                cnid = u32(rec, 8)
        return self.forkdata(rec[88:168]), self.forkdata(rec[168:248])


# -- wrappers ----------------------------------------------------------------------------------------
def macbinary(path):
    d = open(path, "rb").read()
    dl, rl = struct.unpack(">II", d[83:91])
    ro = 128 + ((dl + 127) // 128) * 128
    return d[128:128 + dl], d[ro:ro + rl]


def appledouble_rsrc(path):
    d = open(path, "rb").read()
    for i in range(u16(d, 24)):
        eid, off, ln = struct.unpack(">III", d[26 + 12 * i:38 + 12 * i])
        if eid == 2:
            return d[off:off + ln]
    raise ValueError("no resource fork in %s" % path)


def resources(r):
    doff, moff, _, mlen = struct.unpack(">IIII", r[:16])
    m = r[moff:moff + mlen]
    tl, out = u16(m, 24), {}
    for i in range(u16(m, tl) + 1):
        t, n, ro = struct.unpack(">4sHH", m[tl + 2 + 8 * i:tl + 10 + 8 * i])
        for j in range(n + 1):
            rid, _, a = struct.unpack(">hHI", m[tl + ro + 12 * j:tl + ro + 12 * j + 8])
            o = doff + (a & 0xFFFFFF)
            out[(t, rid)] = r[o + 4:o + 4 + u32(r, o)]
    return out


def adc(src):
    """Apple Data Compression"""
    out, i = bytearray(), 0
    while i < len(src):
        b = src[i]
        if b & 0x80:
            n = (b & 0x7F) + 1
            out += src[i + 1:i + 1 + n]
            i += 1 + n
            continue
        if b & 0x40:
            n, off = (b & 0x3F) + 4, (src[i + 1] << 8) | src[i + 2]
            i += 3
        else:
            n, off = ((b >> 2) & 0xF) + 3, ((b & 3) << 8) | src[i + 1]
            i += 2
        for _ in range(n):
            out.append(out[-off - 1])
    return bytes(out)


def ndif(path, out):
    """a MacBinary DiskCopy 6 image (.img.bin) to a raw volume: the chunk map is resource bcem 128"""
    df, rf = macbinary(path)
    b = resources(rf)[(b"bcem", 128)]
    img = bytearray(u32(b, 0x44) * 512)
    for k in range(u32(b, 0x7C)):
        st, off, ln = struct.unpack(">III", b[0x80 + 12 * k:0x8C + 12 * k])
        sec, typ = st >> 8, st & 0xFF
        if typ == 0xFF:
            break
        if typ in (2, 0x83):
            data = adc(df[off:off + ln]) if typ == 0x83 else df[off:off + ln]
            img[sec * 512:sec * 512 + len(data)] = data
        elif typ:
            raise ValueError("NDIF chunk type %x" % typ)
    open(out, "wb").write(img)


# -- Installer VISE 3.x: the SVCT archive of a Metrowerks updater ----------------------------------------
VISE_TABLE = bytes.fromhex(
    "6ab736ec15d9c873e8389adf2125d0ccfddc16d7e34305c58f48daf23f10236c777cf9a0a3e9ed468bd8ac54ce2d195e"
    "6d7d875dfa5b9be0c7ee9f52a9b90ad1fe78764a3d445a96901f269d581b8e5759c30b6bfc1de6a27f924f40b406724d"
    "f434aad249adef221ab5babf296889933e3204f5dee16ffb67e47e08aff0ab4182ea500f2ac635b3a8cae54c458a97ae"
    "d6662753c91c3c0399c1092e69378d2f60c2a6184e7ab8cfa73a17d59ef184510da464c41eb13098bb7901f6620eb263"
    "91cbff8071e7d400db752cbd393394bc8c3bb6208524882b70836e7b9cbe1447654b5681f8121128eb5574a131f7b013"
    "86dd5f42d30261950c5ca5cdc007e2f3")


def swap(s):
    t = bytearray(s)
    n = len(t) & ~1
    t[0:n:2], t[1:n:2] = t[1:n:2], t[0:n:2]
    return bytes(t)


def inflate(t, want):
    out, off = bytearray(), 0
    while off < len(t) and len(out) < want:
        d = zlib.decompressobj(-15)
        out += d.decompress(t[off:])
        off = (len(t) - len(d.unused_data) + 1) & ~1
        if not d.eof:
            break
    if len(out) < want:
        raise ValueError("short")
    return bytes(out[:want])


def vise_fork(packed, want):
    if not want:
        return b""
    s = swap(packed)
    for t in (s, s.translate(VISE_TABLE)):
        try:
            return inflate(t, want)
        except (ValueError, zlib.error):
            pass
    raise ValueError("VISE fork does not inflate")


def vise(b, name):
    """(data fork, resource fork) of the file NAME in the VISE archive B (an SVCT data fork)"""
    cat = u32(b, 36)
    if b[:4] != b"SVCT" or b[cat:cat + 4] != b"CVCT":
        raise ValueError("not a full Installer VISE archive")
    recs = zlib.decompressobj(-15).decompress(swap(b[cat + 96:])[4:])
    for m in re.finditer(rb"FVCT", recs):
        r = recs[m.start() + 4:m.start() + 190]
        if recs[m.start() + 190:m.start() + 190 + r[118]].decode("mac_roman") != name:
            continue
        off, dpack, dlen, rpack, rlen = u32(r, 96), u32(r, 64), u32(r, 68), u32(r, 72), u32(r, 76)
        if not dlen:
            continue
        return vise_fork(b[off:off + dpack], dlen), vise_fork(b[off + dpack:off + dpack + rpack], rlen)
    raise ValueError("no %s in the VISE archive" % name)
