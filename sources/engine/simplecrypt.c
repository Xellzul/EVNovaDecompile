// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* simplecrypt.c -- the resource XOR cipher (win 0x0046f960). Notes: Research/notes/simplecrypt.md */
#include <stddef.h>
#include "int32.h"

#define kCryptStep 0xdeadbeefUL

#ifdef MAC1010
/* DoEncryption, 1.0.10: on a handle, big-endian only */
OSErr DoEncryption(Handle h, uint32_t key)
{
    OSErr err;
    uint32_t curKey;
    uint32_t *p;
    int32_t size;
    short rem;
    int32_t count;
    uint32_t word;
    unsigned char *tail;

    err = noErr;
    if (h == NULL)
        err = nilHandleErr;
    else if (*h == NULL)
        err = nilHandleErr;
    else {
        curKey = key;
        p = (uint32_t *)*h;
        size = GetHandleSize(h);
        count = size >> 2;
        rem = size - (count << 2);
        while (count--) {
            word = *p ^ curKey;
            *p = word;
            curKey = (curKey + kCryptStep) ^ kCryptStep;
            p++;
        }
        word = *p ^ curKey;
        tail = (unsigned char *)p;
        while (rem--) {
            *tail = word >> 24;
            word <<= 8;
            tail++;
        }
    }
    return err;
}
#else
/* DoEncryption. STATE: MATCH, byte-identical, 68 / 68; mac MATCH on both slices.
   OPEN: none */
// FUNCTION: EVNOVA 0x0046f960
// MAC: _DoEncryption i386 0x000a51cb ppc 0x000b0f40
int32_t DoEncryption(void *buf, uint32_t len, uint32_t key)
{
    short rem;
    uint32_t count;
    uint32_t mask;
    unsigned char *p;
    uint32_t word;
    uint32_t curKey;

    if (buf != NULL && len != 0) {
        p = buf;
        count = len >> 2;
        rem = (len - count * 4);
        curKey = key;
        while (count--) {
#if TARGET_RT_BIG_ENDIAN
            word = curKey;
#else
            word = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
                   | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
#endif
            word ^= *(uint32_t *)p;
            *(uint32_t *)p = word;
            curKey += kCryptStep;
            p += 4;
            curKey ^= kCryptStep;
        }
#if TARGET_RT_BIG_ENDIAN
        mask = curKey;
#else
        mask = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
               | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
#endif
        mask ^= *(uint32_t *)p;
#if TARGET_RT_BIG_ENDIAN
        while (rem--) {
            *p = mask >> 24;
            p += 1;
            mask <<= 8;
        }
#else
        while (rem--) {
            *p = mask & 0xff;
            p += 1;
            mask >>= 8;
        }
#endif
    }
    return 0;
}
#endif
