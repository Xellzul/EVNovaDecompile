/* hashload1010.inc -- HashLoad's body on mac 1.0.10, one logic with win's in another wording; win and mac 1.1.1 keep theirs in athashtbl.c. */
{
    char message[0x100];
    ULong sepWide;
    ULong wide;
    ULong kind;
    ULong valueLen;
    void *t;
    ULong keyStart;
    ULong keyLen;
    ULong sep;
    ULong valueStart;
    long err, e;
    ULong pos;
    ULong state;
    ULong c;
    ULong keyClass;
    void **value;
    char *data;
    unsigned char bad;
    ULong length;
    ULong quote;
    unsigned char wasLocked;
    HashTable *table;

    err = 0;
    t = 0;
    table = 0;
    keyClass = 0;
    value = 0;
    data = 0;
    wasLocked = 0;
    bad = 0;
    t = HashCreate(flags);
    if (t == 0) {
        err = kOutOfMemory;
        goto done;
    }
    table = t;
    wasLocked = IndirectSetLock(text, 1);
    wide = flags & kKindUTF16;
    if (wide != 0)
        length = UnicodeGetCharCountFromBytes(*text, IndirectGetSize(text));
    else
        length = IndirectGetSize(text);
    kind = flags & kKindMask;
    sepWide = flags & kKeyWide;
    keyClass = table->keyClass;
    data = *text;
    keyStart = keyLen = valueStart = valueLen = 0;
    pos = 0;
    state = kParseSkip;
    sep = 0;

    for (; pos <= length; pos += wide != 0 ? CHAR_SIZE(c) : 1) {
        if (wide != 0)
            c = pos < length ? UnicodeGetCharByIndex((const unsigned short *)data, pos) : 0;
        else
            c = pos < length ? (unsigned char)data[pos] : 0;
    dispatch:
        switch (state) {
        case kParseSkip:
            if (c == 0 || CHAR_IN(wide, c, kClassBlank))
                break;
            if (KEY_CHAR(wide, c, keyClass)) {
                keyStart = pos;
                state = kParseKey;
                break;
            }
            bad = 1;
            break;

        case kParseKey:
            if (KEY_CHAR(wide, c, keyClass))
                break;
            keyLen = pos - keyStart;
            if (c == 0)
                break;
            if (SEPARATOR(wide, c, sepWide)) {
                state = kParseSeparator;
                sep = c;
                break;
            }
            if (CHAR_IN(wide, c, kClassBlank)) {
                state = kParseAfterKey;
                sep = 0;
                break;
            }
            bad = 1;
            break;

        case kParseAfterKey:
            if (CHAR_IN(wide, c, kClassBlank))
                break;
            if (SEPARATOR(wide, c, sepWide)) {
                state = kParseSeparator;
                sep = c;
                break;
            }
            if (c == 0 || KEY_CHAR(wide, c, keyClass)) {
                if (wide != 0) {
                    err = _HashInsertAppend(t, data + keyStart * 2, keyLen,
                                       0, 0, sep, 0, kind);
                    if (err == kDuplicateKey)
                        err = 0;
                } else {
                    err = _HashInsertAppend(t, data + keyStart, keyLen, 0, 0, sep, 0,
                                       kind);
                    if (err == kDuplicateKey)
                        err = 0;
                }
                require_noerr(err, done);
                keyLen = valueLen = 0;
                keyStart = pos;
                state = kParseKey;
                break;
            }
            bad = 1;
            break;

        case kParseSeparator:
            if (IS_CHAR(wide, c, '='))
                break;
            if (CHAR_IN(wide, c, kClassSeparator)) {
                bad = 1;
                break;
            }
            if (CHAR_IN(wide, c, kClassBlank)) {
                state = kParseAfterSeparator;
                break;
            }
            valueStart = pos;
            quote = c;
            state = QUOTE_CHAR(wide, c) ? kParseQuoted : kParseValue;
            break;

        case kParseAfterSeparator:
            if (c == 0 || CHAR_IN(wide, c, kClassBlank))
                break;
            valueStart = pos;
            quote = c;
            state = QUOTE_CHAR(wide, c) ? kParseQuoted : kParseValue;
            break;

        case kParseValue:
        case kParseQuoted:
            if (state == kParseValue && c != 0 && !CHAR_IN(wide, c, kClassBlank))
                break;
            if (state == kParseQuoted && c != 0 && c != quote)
                break;
            valueLen = pos - valueStart;
            if (wide != 0) {
                if (state == kParseQuoted) {
                    valueStart += CHAR_SIZE(c);
                    valueLen -= CHAR_SIZE(c);
                }
                if (value == 0) {
                    value = IndirectAllocate(0);
                    if (value == 0) {
                        err = kOutOfMemory;
                        goto done;
                    }
                }
                valueLen <<= 1;
                e = IndirectSetSize(value, valueLen);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                /* BUG: the value's character index is used as a byte offset */
                MemoryCopy(*value, data + valueStart, valueLen);
                e = _HashDataUnicode(value, 0);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                IndirectSetLock(value, 1);
                err = _HashInsertAppend(t, data + keyStart * 2, keyLen, *value, IndirectGetSize(value),
                                   sep, 0, kind);
                if (err == kDuplicateKey)
                    err = 0;
                IndirectSetLock(value, 0);
                if (err == 0)
                    goto setsize;
                goto done;
            } else {
                if (state == kParseQuoted) {
                    valueStart++;
                    valueLen--;
                }
                if (value == 0) {
                    value = IndirectAllocate(0);
                    if (value == 0) {
                        err = kOutOfMemory;
                        goto done;
                    }
                }
                e = IndirectSetSize(value, valueLen);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                MemoryCopy(*value, data + valueStart, valueLen);
                e = _HashDataDecode(value);
                if (e != 0) {
                    err = e;
                    goto done;
                }
                IndirectSetLock(value, 1);
                err = _HashInsertAppend(t, data + keyStart, keyLen, *value, IndirectGetSize(value),
                                   sep, 0, kind);
                if (err == kDuplicateKey)
                    err = 0;
                IndirectSetLock(value, 0);
                require_noerr(err, done);
            }
setsize:
            e = IndirectSetSize(value, 0);
            if (e != 0) {
                err = e;
                goto done;
            }
            keyLen = valueLen = 0;
            state = kParseSkip;
            break;
        }

        if (bad != 0) {
            StringFormatSafe(message, sizeof(message),
                         c >= kMaxAscii ? "Unexpected character 0x%lX at offset %ld"
                         : (c <= ' ' || c >= kMaxAscii) ? "Unexpected character 0x%2.2lX at offset %ld"
                         : "Unexpected character '%c' at offset %ld", c, pos);
            bad = 0;
            if (wide != 0 && c != ' ') {
                c = ' ';
                goto dispatch;
            }
            if (c != ' ') {
                c = ' ';
                goto dispatch;
            }
        }
    }

    if (c == 0 && keyLen != 0) {
        if (wide != 0) {
            err = _HashInsertAppend(t, data + keyStart * 2, keyLen, 0, 0, sep, 0,
                               kind);
            if (err == kDuplicateKey)
                err = 0;
        } else {
            err = _HashInsertAppend(t, data + keyStart, keyLen, 0, 0, sep, 0,
                               kind);
            if (err == kDuplicateKey)
                err = 0;
        }
        e = err;
        if (0 != e)
            err = e;
    }

done:
    if (value != 0)
        IndirectDeallocate(value);
    if (wasLocked == 0)
        IndirectSetLock(text, 0);
    if (err != 0 && t != 0) {
        HashDispose(t);
        t = 0;
    }
    return (HashTable *)t;
}
