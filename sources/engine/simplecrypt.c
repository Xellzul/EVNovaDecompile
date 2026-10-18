// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off -proc generic
/* simplecrypt.c -- the resource XOR cipher (win 0x0046f960). Notes: Research/notes/simplecrypt.md */
#include <stddef.h>

#define kCryptStep 0xdeadbeefUL

/* DoEncryption. STATE: MATCH, byte-identical, 68 / 68.
   OPEN: none */
// FUNCTION: EVNOVA 0x0046f960
long DoEncryption(void *buf, unsigned long len, unsigned long key)
{
    unsigned short rem;
    unsigned long count;
    unsigned long mask;
    unsigned char *p;
    unsigned long word;
    unsigned long curKey;

    if (buf != NULL && len != 0) {
        p = (unsigned char *)buf;
        count = len >> 2;
        rem = (len - count * 4);
        curKey = key;
        while (count--) {
            word = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
                   | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
            word ^= *(unsigned long *)p;
            *(unsigned long *)p = word;
            curKey += kCryptStep;
            p += 4;
            curKey ^= kCryptStep;
        }
        mask = ((curKey >> 24) & 0xff) | ((curKey >> 8) & 0xff00)
               | ((curKey & 0xff00) << 8) | ((curKey & 0xff) << 24);
        mask ^= *(unsigned long *)p;
        while (rem--) {
            *p = mask & 0xff;
            p += 1;
            mask >>= 8;
        }
    }
    return 0;
}
