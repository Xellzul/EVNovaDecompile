/* indirectdecompress1010.inc -- IndirectDecompress's body on mac 1.0.10; win and mac 1.1.1 keep theirs in atplatheap.c. */
{
    void **out;
    long err;
    ULong size;
    ULong step;
    ULong w;
    unsigned char *p;
    ULong pos;
    ULong outPos;
    ULong cap;
    unsigned short width;
    int zerr;
    unsigned char *src;
    unsigned char zopen;
    unsigned short count;
    unsigned char locked;
    unsigned char *q;
    ULong n;
    unsigned int c;
    unsigned int b0;
    unsigned int b1;
    long r;
    z_stream z;

    err = 0;
    out = 0;
    locked = 0;
    zopen = 0;
    SystemCheckCallback();
    _AtomicCheckCritical();
    _ThreadCheckCritical();
    if (info != 0)
        MemoryClear(info, sizeof(PackInfo));
    if ((unsigned char)IndirectGetLock(handle) != 0) {
        err = kBadArgument;
        goto done;
    }
    locked = 1;
    if (IndirectGetSize(handle) == 0)
        tag = kTagNone;

    if (tag == kTagNone) {
    } else if (tag == kTagZlib) {
        size = IndirectGetSize(handle) * 2;
        if (size < kGrowStep)
            size = kGrowStep;
        out = IndirectAllocate(size);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(out, 1);
        IndirectSetLock(handle, 1);
        z.zalloc = Z_NULL;
        z.zfree = Z_NULL;
        z.opaque = Z_NULL;
        z.next_in = *handle;
        z.avail_in = IndirectGetSize(handle);
        z.next_out = *out;
        z.avail_out = IndirectGetSize(out);
        zerr = inflateInit(&z);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_MEM_ERROR:
            err = zerr;
            err = kOutOfMemory;
            goto done;
        case Z_VERSION_ERROR:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
        zopen = 1;
        while (z.avail_in != 0 && zerr != Z_STREAM_END) {
            zerr = inflate(&z, Z_NO_FLUSH);
            switch (zerr) {
            case Z_OK:
            case Z_STREAM_END:
                break;
            case Z_STREAM_ERROR:
                err = zerr;
                err = kBadArgument;
                goto done;
            case Z_BUF_ERROR:
            default:
                err = zerr;
                err = kBadArgument;
                goto done;
            }
            if (zerr != Z_STREAM_END) {
                IndirectSetLock(out, 0);
                size = z.total_out + kGrowStep;
                r = IndirectSetSize(out, size);
                if (r != 0) {
                    err = r;
                    goto done;
                }
                IndirectSetLock(out, 1);
                z.next_out = (Bytef *)*out + z.total_out;
                z.avail_out = size - z.total_out;
            }
        }
        IndirectSetLock(handle, 0);
        IndirectSetLock(out, 0);
        r = IndirectSetSize(out, z.total_out);
        if (r != 0) {
            err = r;
            goto done;
        }
        zopen = 0;
        zerr = inflateEnd(&z);
        switch (zerr) {
        case Z_OK:
            break;
        case Z_STREAM_ERROR:
            err = zerr;
            err = kBadArgument;
            goto done;
        case Z_ERRNO:
        default:
            err = zerr;
            err = kBadArgument;
            goto done;
        }
    } else if (tag == kTagPack || tag == kTagPackWide) {
        width = 1;
        out = IndirectAllocate(0);
        if (out == 0) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(handle, 1);
        if (tag == kTagPackWide)
            width = 2;
        size = IndirectGetSize(handle);
        src = *handle;
        w = width;
        step = w + 1;
        pos = outPos = cap = 0;
        while (pos < size) {
            p = src + pos;
            c = *p;
            if (c == 0x80) {
                pos++;
                continue;
            }
            if ((c & 0x80) != 0) {
                count = 0x101 - c;
                if (pos + w + 1 > size) {
                    err = kBadArgument;
                    goto done;
                }
                if (cap - outPos < count * w) {
                    cap += kGrowStep;
                    r = IndirectSetSize(out, cap);
                    if (r != 0) {
                        err = r;
                        goto done;
                    }
                }
                if (width == 1) {
                    b0 = p[1];
                    while (count--)
                        (*(unsigned char **)out)[outPos++] = b0;
                } else {
                    b0 = p[1];
                    b1 = p[2];
                    while (count--) {
                        (*(unsigned char **)out)[outPos++] = b0;
                        (*(unsigned char **)out)[outPos++] = b1;
                    }
                }
                pos += step;
            } else {
                count = c + 1;
                n = count * w;
                if (pos + n > size) {
                    err = kBadArgument;
                    goto done;
                }
                if (cap - outPos < n) {
                    cap += kGrowStep;
                    r = IndirectSetSize(out, cap);
                    if (r != 0) {
                        err = r;
                        goto done;
                    }
                }
                if (width == 1) {
                    pos = count + pos;
                    q = p + 1;
                    pos++;
                    while (count--)
                        (*(unsigned char **)out)[outPos++] = *q++;
                } else {
                    pos = (count << 1) + pos;
                    q = p + 1;
                    pos++;
                    while (count--) {
                        (*(unsigned char **)out)[outPos++] = q[0];
                        (*(unsigned char **)out)[outPos++] = q[1];
                        q += 2;
                    }
                }
            }
        }
        IndirectSetLock(handle, 0);
        IndirectSetLock(out, 0);
        r = IndirectSetSize(out, outPos);
        if (r != 0) {
            err = r;
            goto done;
        }
    } else {
        if (tag == kTagAny || tag == kTagFast || tag == kTagBest) {
            err = kUnsupported;
            goto done;
        }
        err = kUnsupported;
        goto done;
    }

    if (info != 0) {
        info->magic = kPackMagic;
        info->tag = tag;
        info->packedSize = IndirectGetSize(handle);
        info->packedSum = MemoryChecksum(*handle, IndirectGetSize(handle));
        if (tag == kTagNone) {
            info->size = IndirectGetSize(handle);
            info->sum = MemoryChecksum(*handle, IndirectGetSize(handle));
        } else {
            info->size = IndirectGetSize(out);
            info->sum = MemoryChecksum(*out, IndirectGetSize(out));
        }
    }
    if (out != 0) {
        size = IndirectGetSize(out);
        r = IndirectSetSize(handle, size);
        if (r != 0) {
            err = r;
            goto done;
        }
        MemoryCopy(*handle, *out, size);
    }
done:
    if (zopen)
        inflateEnd(&z);
    if (locked)
        IndirectSetLock(handle, 0);
    if (out != 0)
        IndirectDeallocate(out);
    return err;
}
