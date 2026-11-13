/* picthandlebits1010.inc -- _PICT_HandleBits' body on mac 1.0.10: the frame kept in imageBounds, bits under 8 deep unpacked, a packed 32-bit PixMap with or without its alpha plane; win and mac 1.1.1 keep theirs in itpict.c. */
{
    void **packed;
    int32_t err;
    int32_t e;
    ULong y;
    ULong x;
    ULong rowBytes;
    ULong n;
    ULong v;
    ULong c;
    unsigned char *row;
    ITColor *rgb;
    ULong height;
    ULong width;
    void **region;
    PictPixMap map;
    PictColorTable table;
    PictFrame srcRect;
    PictFrame dstRect;
    ITRect src;
    ITRect dst;
    ITPalette palette;
    unsigned short packType;
    short mode;
    unsigned char count8;
    unsigned short count16;
    short depth;

    err = 0;
    packed = NULL;
    row = NULL;
    rgb = NULL;
    region = NULL;
    packType = kPackTypeUnknown;
    switch (opcode) {
    case kOpBitsRect:
    case kOpBitsRgn:
        map.baseAddr = 0;
        e = _PICT_ReadPixMapTable(stream, &map, &table);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        packType = kPackTypeNone;
        break;
    case kOpPackBitsRect:
    case kOpPackBitsRgn:
        map.baseAddr = 0;
        e = _PICT_ReadPixMapTable(stream, &map, &table);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpPackBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        if ((map.rowBytes & kRowBytesMask) < kPackMinRowBytes)
            packType = kPackTypeNone;
        else
            packType = kPackTypeDefault;
        break;
    case kOpDirectBitsRect:
    case kOpDirectBitsRgn:
        table.size = 0xffff;
        e = DT_BufferRead(stream, &map.baseAddr, 4);
        CheckErr(e);
        e = _PICT_ReadPixMap(stream, &map);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&srcRect);
        CheckErr(e);
        e = _PICT_ReadRect(stream, (short *)&dstRect);
        CheckErr(e);
        e = DT_BufferRead(stream, &mode, 2);
        CheckErr(e);
        if (opcode == kOpDirectBitsRgn) {
            e = _PICT_ReadRegion(stream, &region);
            CheckErr(e);
        }
        if (map.packType == kPackTypeNone || (map.rowBytes & kRowBytesMask) < kPackMinRowBytes)
            packType = kPackTypeNone;
        else
            packType = map.packType;
        break;
    }

    height = map.bounds.bottom - map.bounds.top;
    width = map.bounds.right - map.bounds.left;
    rowBytes = map.rowBytes & kRowBytesMask;
    DT_SetRect(&src, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
    DT_SetRect(&dst, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
    if (!DT_IsRectEqual(&src, &s->imageBounds)) {
        if (s->image != NULL)
            _DeferredImageDispose(s);
        s->imageBounds = src;
    }

    switch (map.pixelSize) {
    case 1:
    case 2:
    case 4:
    case 8:
        palette.count = table.size + 1;
        for (y = 0; y < palette.count; y++) {
            palette.entry[y].alpha = 0xff;
            palette.entry[y].red = table.entry[y].red >> 8;
            palette.entry[y].green = table.entry[y].green >> 8;
            palette.entry[y].blue = table.entry[y].blue >> 8;
        }
        if (s->image != NULL && s->info.depth != 8)
            _DeferredImageDispose(s);
        if (s->image == NULL) {
            e = _DeferredImageCreate(s, 8, palette.count != 0 ? &palette : NULL, 0);
            CheckErr(e);
        }
        row = MemoryAllocate(rowBytes * 8 / map.pixelSize);
        if (row == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    case 16:
    case 32:
        if (s->image != NULL && s->info.depth != map.pixelSize)
            _DeferredImageDispose(s);
        if (s->image == NULL) {
            e = _DeferredImageCreate(s, map.pixelSize, NULL, 0);
            CheckErr(e);
        }
        rgb = MemoryAllocate((rowBytes / s->info.bytesPerPixel + 1) * sizeof(ITColor));
        if (rgb == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        break;
    default:
        err = kPictBadData;
        goto done;
    }

    switch (packType) {
    case kPackTypeNone:
        packed = IndirectAllocate(rowBytes);
        if (packed == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(packed, 1);
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, *packed, rowBytes);
            CheckErr(e);
            if (map.pixelSize == 32) {
                /* BUG: steps x by the byte stride, so only every 4th pixel is written and rgb overruns */
                for (x = 0; x < width; x += 4) {
                    rgb[x].alpha = 0xff;
                    rgb[x].red = HandleBytes(packed)[x + 1];
                    rgb[x].green = HandleBytes(packed)[x + 2];
                    rgb[x].blue = HandleBytes(packed)[x + 3];
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (map.pixelSize == 16) {
                for (x = 0; x < width; x++) {
                    v = FromBE16(((unsigned short *)*packed)[x]);
                    rgb[x].alpha = 0xff;
                    c = (v >> 10) & 0x1f;
                    rgb[x].red = Expand5(c);
                    c = (v >> 5) & 0x1f;
                    rgb[x].green = Expand5(c);
                    n = v & 0x1f;
                    rgb[x].blue = Expand5(n);
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (map.pixelSize == 8) {
                MemoryCopy(row, *packed, width);
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            } else {
                /* BUG: (depth << 1) - 1 is the 4-bit mask 7, not 15 */
                depth = map.pixelSize;
                for (x = 0; x * 8 < width * depth; x++) {
                    c = ((unsigned char *)*packed)[x];
                    for (n = 0, v = (depth << 1) - 1; n * depth < 8; n++, c <<= depth) {
                        ULong bits = c >> (8 - depth);
                        row[x * 8 / depth + n] = bits & v;
                    }
                }
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            }
        }
        break;
    case kPackTypeNoAlpha:
        n = rowBytes * 3 / 4;
        packed = IndirectAllocate(n);
        if (packed == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        IndirectSetLock(packed, 1);
        for (y = 0; y < height; y++) {
            e = DT_BufferRead(stream, *packed, n);
            CheckErr(e);
            /* BUG: steps x by the byte stride, so only every 3rd pixel is written and rgb overruns */
            for (x = 0; x < width; x += 3) {
                rgb[x].alpha = 0xff;
                rgb[x].red = HandleBytes(packed)[x];
                rgb[x].green = HandleBytes(packed)[x + 1];
                rgb[x].blue = HandleBytes(packed)[x + 2];
            }
            DT_ImageSetRawColors(s->image, s->mask, y, rgb);
        }
        break;
    case kPackTypeDefault:
    case kPackTypeWords:
    case kPackTypeChannels:
        packed = IndirectAllocate(0);
        if (packed == NULL) {
            err = kOutOfMemory;
            goto done;
        }
        for (y = 0; y < height; y++) {
            if (rowBytes <= kPackCountByte) {
                e = DT_BufferRead(stream, &count8, 1);
                CheckErr(e);
                n = count8;
            } else {
                e = DT_BufferRead(stream, &count16, 2);
                CheckErr(e);
                n = FromBE16(count16);
            }
            e = IndirectSetSize(packed, n);
            CheckErr(e);
            IndirectSetLock(packed, 1);
            e = DT_BufferRead(stream, *packed, n);
            CheckErr(e);
            IndirectSetLock(packed, 0);
            if (map.pixelSize == 32) {
                e = IndirectDecompress(packed, kTagPackBytes, NULL);
                CheckErr(e);
                if (map.cmpCount == 3) {
                    for (x = 0; x < width; x++) {
                        rgb[x].alpha = 0xff;
                        rgb[x].red = HandleBytes(packed)[x];
                        rgb[x].green = HandleBytes(packed)[(rowBytes >> 2) + x];
                        rgb[x].blue = HandleBytes(packed)[(rowBytes >> 1) + x];
                    }
                } else {
                    for (x = 0; x < width; x++) {
                        rgb[x].alpha = 0xff;
                        rgb[x].red = HandleBytes(packed)[(rowBytes >> 2) + x];
                        rgb[x].green = HandleBytes(packed)[(rowBytes >> 1) + x];
                        rgb[x].blue = HandleBytes(packed)[rowBytes * 3 / 4 + x];
                    }
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (map.pixelSize == 16) {
                e = IndirectDecompress(packed, kTagPackWords, NULL);
                CheckErr(e);
                for (x = 0; x < width; x++) {
                    v = FromBE16(((unsigned short *)*packed)[x]);
                    rgb[x].alpha = 0xff;
                    c = (v >> 10) & 0x1f;
                    rgb[x].red = Expand5(c);
                    c = (v >> 5) & 0x1f;
                    rgb[x].green = Expand5(c);
                    n = v & 0x1f;
                    rgb[x].blue = Expand5(n);
                }
                DT_ImageSetRawColors(s->image, s->mask, y, rgb);
            } else if (map.pixelSize == 8) {
                e = IndirectDecompress(packed, kTagPackBytes, NULL);
                CheckErr(e);
                MemoryCopy(row, *packed, width);
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            } else {
                e = IndirectDecompress(packed, kTagPackBytes, NULL);
                CheckErr(e);
                depth = map.pixelSize;
                for (x = 0; x * 8 < width * depth; x++) {
                    c = ((unsigned char *)*packed)[x];
                    for (n = 0, v = (depth << 1) - 1; n * depth < 8; n++, c <<= depth) {
                        ULong bits = c >> (8 - depth);
                        row[x * 8 / depth + n] = bits & v;
                    }
                }
                DT_ImageSetRawPixels(s->image, s->mask, y, row);
            }
        }
        break;
    default:
        err = kPictBadData;
        break;
    }
done:
    if (row != NULL)
        MemoryDeallocate(row);
    if (rgb != NULL)
        MemoryDeallocate(rgb);
    if (packed != NULL)
        IndirectDeallocate(packed);
    if (region != NULL)
        IndirectDeallocate(region);
    return err;
}
