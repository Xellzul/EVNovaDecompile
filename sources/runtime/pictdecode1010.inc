/* pictdecode1010.inc -- PICT_Decode's body on mac 1.0.10; win and mac 1.1.1 keep theirs in itpict.c. */
{
    long err;
    long e;
    unsigned short version;
    long length;
    unsigned char unused;
    unsigned char skip[kPictSkipChunk];
    unsigned short picSize;
    PictFrame frame;
    unsigned short opcode;
    char b;
    PictState s;
    PictOpcode entry;

    err = 0;
    unused = 0;
    picSize = 0;
    version = 0;
    *imageOut = NULL;
    if (maskOut != NULL)
        *maskOut = NULL;
    if (trans != NULL)
        DT_AlphaInfoDefaults(trans);
    if (bg != NULL)
        DT_FileInfoDefaults(bg);
    MemoryClear(&s, sizeof(s));
    if (!_PICT_CheckFileType(*stream->data, stream->size)) {
        e = DT_BufferRead(stream, skip, kPictFileHeader);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    e = DT_BufferRead(stream, &picSize, 2);
    if (e != 0) {
        err = e;
        goto done;
    }
    picSize = FromBE16(picSize);
    e = _PICT_ReadRect(stream, (short *)&frame);
    if (e != 0) {
        err = e;
        goto done;
    }
    do {
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
    } while (b == 0);
    if (b != kPictVersionOp) {
        err = kPictBadData;
        goto done;
    }
    e = DT_BufferRead(stream, &b, 1);
    if (e != 0) {
        err = e;
        goto done;
    }
    switch (b) {
    case kPictVersion1:
        version = 1;
        break;
    case kPictVersion2:
        e = DT_BufferRead(stream, &b, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (b != (char)kPictVersion2Tail) {
            err = kPictBadData;
            goto done;
        }
        version = 2;
        break;
    default:
        err = kPictBadData;
        goto done;
    }
    s.version = version;
    DT_SetRect(&s.bounds, frame.left, frame.top, frame.right, frame.bottom);
    s.imageBounds = s.bounds;
    s.trans = trans;
    s.bg = bg;

    for (;;) {
        PictOpcode wordData = PICT_OP(kOpWordLength, NULL);
        PictOpcode longData = PICT_OP(kOpLongLength, NULL);
        PictOpcode noData = PICT_OP(0, NULL);
        PictOpcode wordFixed = PICT_OP(2, NULL);
        PictOpcode longFixed = PICT_OP(4, NULL);
        unsigned short wordLength;
        ULong longLength;
        unsigned short wordSize;
        ULong longSize;
        unsigned short commentSize;
        PictOpcode *p = &entry;

        e = _PICT_ReadOpCode(stream, version, &opcode);
        if (e != 0) {
            err = e;
            goto done;
        }
        if (opcode <= kPictLastTableOp)
            *p = gDT_PictOpTable[opcode];
        else if (opcode <= kPictLastWordData)
            *p = wordData;
        else if (opcode <= kPictLastNoData)
            *p = noData;
        else if (opcode <= kPictLastLongData)
            *p = longData;
        else if (opcode == kPictOpEndPic)
            break;
        else if (opcode == kPictOpVersion) {
            p->length = 2;
            p->handler = NULL;
            p->unsupported = 0;
            p->name = "versionOp";
        } else if (opcode == kPictOpHeader) {
            p->length = kPictHeaderLength;
            p->handler = NULL;
            p->unsupported = 0;
            p->name = "headerOp";
        } else if (opcode <= kPictLastReserved) {
            p->length = (opcode & 0x7f00) >> 7;
            p->handler = NULL;
            p->unsupported = 0;
            p->name = "reserved";
        } else if (opcode <= kPictLastReservedNoData)
            *p = noData;
        else if (opcode == kPictOpCompressedQT) {
            p->length = kOpLongLength;
            p->handler = NULL;
            p->unsupported = 0;
            p->name = "compressedQuicktime";
            err = kPictUnsupported;
            goto done;
        } else if (opcode == kPictOpUncompressedQT) {
            p->length = kOpLongLength;
            p->handler = NULL;
            p->unsupported = 0;
            p->name = "uncompressedQuicktime";
            err = kPictUnsupported;
            goto done;
        } else
            /* BUG: 0x8001..0x80ff take a long length too, where Apple gives them no data */
            *p = longData;

        if (entry.unsupported != 0) {
            err = kPictUnsupported;
            goto done;
        }
        if (entry.handler != NULL) {
            e = entry.handler(stream, &s, &entry, opcode);
            if (e != 0) {
                err = e;
                goto done;
            }
            continue;
        }

        if (entry.length == kOpHandler) {
            err = kPictBadData;
            goto done;
        }
        else if (entry.length == kOpWordLength) {
            e = DT_BufferRead(stream, &wordLength, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE16(wordLength);
            if (length < 0) {
                err = kPictBadData;
                goto done;
            }
        } else if (entry.length == kOpLongLength) {
            e = DT_BufferRead(stream, &longLength, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE32(longLength);
            if (length < 0) {
                err = kPictBadData;
                goto done;
            }
        } else if (entry.length == kOpWordSize) {
            e = DT_BufferRead(stream, &wordSize, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE16(wordSize) - 2;
        } else if (entry.length == kOpLongSize) {
            e = DT_BufferRead(stream, &longSize, 4);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE32(longSize) - 4;
        } else if (entry.length == kOpDHText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = (unsigned char)b;
        } else if (entry.length == kOpDHDVText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = (unsigned char)b;
        } else if (entry.length == kOpLongText) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = (unsigned char)b;
        } else if (entry.length == kOpLongComment) {
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &b, 1);
            if (e != 0) {
                err = e;
                goto done;
            }
            e = DT_BufferRead(stream, &commentSize, 2);
            if (e != 0) {
                err = e;
                goto done;
            }
            length = FromBE16(commentSize);
        } else
            /* BUG: also overwrites the -2 / -3 lengths read above, so their skip runs on
               0xfffffffe / 0xfffffffd until a stream error */
            length = entry.length;

        while (length > sizeof(skip)) {
            e = DT_BufferRead(stream, skip, sizeof(skip));
            if (e != 0) {
                err = e;
                goto done;
            }
            length -= sizeof(skip);
        }
        e = DT_BufferRead(stream, skip, length);
        if (e != 0) {
            err = e;
            goto done;
        }
    }

    if (s.image == NULL) {
        e = _DeferredImageCreate(&s, kPictBlankDepth, NULL, 1);
        if (e != 0) {
            err = e;
            goto done;
        }
    }
    if (!DT_IsRectEqual(&s.bounds, &s.imageBounds)) {
        if (s.locked) {
            DT_ImageUnlock(s.image, s.mask);
            s.locked = 0;
        }
        e = DT_ImageResize(s.image, s.mask, &s.bounds);
        if (e != 0)
            err = e;
    }
done:
    if (s.locked)
        DT_ImageUnlock(s.image, s.mask);
    if (err != 0 && s.image != NULL)
        DT_ImageDispose(s.image);
    if (err != 0 && s.mask != NULL)
        DT_ImageDispose(s.mask);
    if (err == 0) {
        *imageOut = s.image;
        if (maskOut != NULL)
            *maskOut = s.mask;
    }
    return err;
}
