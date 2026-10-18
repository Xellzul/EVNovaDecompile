// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: its platform, network, file and interface parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atnetaddr.c -- the Winsock layer's network addresses: family, copy, compare, empty, clear, IPv4 in and out, as text. Notes: Research/notes/atnet.md */

#include <string.h>

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

/* AddressToNetworkStack (an IPX address: 1 for IPX, 0 for Inet or any other family). STATE: MATCH
   100%, exe 28 / obj 28, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052917a
Word16 AddressToNetworkStack(const NetAddress *a)
{
    long long family;
    long err, e;
    long result;

    e = err = 0;
    result = 0;
    family = a->family & kNetFamilyMask;
    if (family == kNetFamilyInet)
        result = 0;
    else if (family == kNetFamilyIPX)
        result = 1;
    return result;
}

/* AddressDuplicate (copy an address, its length field first). STATE: MATCH 100%, exe 16 / obj
   16, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005291c5
void AddressDuplicate(void *to, const NetAddress *src)
{
    NetAddress *dst;
    ULong length;
    long err;

    dst = to;
    err = 0;
    length = src->kind + sizeof(long);
    if (length != 0)
        memmove(dst, src, length);
}

/* AddressCompare (two network addresses equal). STATE: MATCH 100%, exe 78 / obj 78,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x005291e8
unsigned char AddressCompare(const void *refA, const void *refB)
{
    long err;
    unsigned char same;
    const NetAddress *a;
    const NetAddress *b;

    err = 0;
    a = refA;
    b = refB;
    same = 0;
    if ((a->family & kNetFamilyMask) == (b->family & kNetFamilyMask)) {
        switch (a->family & kNetFamilyMask) {
        case kNetFamilyNone:
            same = 1;
            break;
        case kNetFamilyInet:
            same = ((const ULong *)a)[2] == ((const ULong *)b)[2] && a->port == b->port;
            break;
        case kNetFamilyIPX:
            same = a->port == b->port && a->node[0] == b->node[0] && a->node[1] == b->node[1];
            break;
        }
    }
    return same;
}

/* AddressIsEmpty (an address with nothing in it). STATE: MATCH 100%, exe 16 / obj 16,
   byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x005292b2
unsigned char AddressIsEmpty(const NetAddress *a)
{
    long err, e;
    long result;

    e = err = 0;
    result = 1;
    if ((ULong)a->kind >= 2 && (a->family & kNetFamilyMask) != 0)
        result = 0;
    return result;
}

/* AddressComposeEmpty (clear an address). STATE: MATCH 100%, exe 12 / obj 12, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x005292db
void AddressComposeEmpty(NetAddress *a)
{
    long *p;
    long err;

    p = (long *)a;
    err = 0;
    *p++ = 0;
    memset(p, 0, sizeof(NetAddress) - sizeof(long));
}

#define kSockaddrInLen  0x10

/* AddressComposeTCPIP (an IPv4 address from a port and an address, both in network order).
   STATE: MATCH 100%, exe 16 / obj 16, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x005292f6
void AddressComposeTCPIP(void *ref, unsigned short port, ULong ip)
{
    NetAddress *a;
    ULong *p;
    long err;

    a = (NetAddress *)ref;
    err = 0;
    a->kind = kSockaddrInLen;
    a->family = kNetFamilyInet;
    a->port = port;
    ((ULong *)a)[2] = ip;
    p = (ULong *)a;
    p += 3;
    memset(p, 0, 2 * sizeof(ULong));
}

/* AddressDecomposeTCPIP (an IPv4 address's port and address). STATE: MATCH 100%, exe 25 / obj 25,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00529323
void AddressDecomposeTCPIP(const void *ref, unsigned short *port, ULong *ip)
{
    const NetAddress *a;
    long err;

    a = (const NetAddress *)ref;
    err = 0;
    if (port != 0)
        *port = 0;
    if (ip != 0)
        *ip = 0;
    if (port != 0)
        *port = a->port;
    if (ip != 0)
        *ip = ((const ULong *)a)[2];
}

#define HIGH_DIGIT(b)   (digits[((b) >> 4) & 0xf])
#define LOW_DIGIT(b)    (digits[(b) & 0xf])

static inline char *AppendString(char *dst, const char *src)
{
    char *start;

    start = dst;
    while (*dst++ != 0)
        ;
    dst--;
    while ((*dst++ = *src++) != 0)
        ;
    dst = start;
    return dst;
}

/* AddressTranscribe (an address as text: "a.b.c.d[:port]" for IPv4, "net.node[:node]" in hex
   for IPX; any other family leaves out untouched). STATE: MATCH 100%, exe 242 / obj 242,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x0052935b
void AddressTranscribe(const void *address, char *out)
{
    const NetAddress *a;
    char number[0x100];
    unsigned char bytes[0x100];
    const char *digits;
    ULong i;
    long err;

    a = address;
    err = 0;
    switch (a->family & kNetFamilyMask) {
    case kNetFamilyInet:
        memmove(bytes, a->node, kOctets);
        out[0] = 0;
        for (i = 0; i < kOctets; i++) {
            if (i != 0)
                AppendString(out, ".");
            StringFromNumber(number, bytes[i]);
            AppendString(out, number);
        }
        i = Swap16(a->port);
        if (i != 0) {
            AppendString(out, ":");
            StringFromNumber(number, i);
            AppendString(out, number);
        }
        break;
    case kNetFamilyIPX:
        digits = "0123456789ABCDEF";
        if (Swap16(a->port) != 0) {
            memmove(bytes, &a->port, sizeof(a->port));
            out[0] = HIGH_DIGIT(bytes[0]);
            out[1] = LOW_DIGIT(bytes[0]);
            out[2] = HIGH_DIGIT(bytes[1]);
            out[3] = LOW_DIGIT(bytes[1]);
            out += 4;
        } else {
            *out++ = digits[0];
        }
        out[0] = '.';
        memmove(bytes, &a->node[0], 1);
        out[1] = HIGH_DIGIT(bytes[0]);
        out[2] = LOW_DIGIT(bytes[0]);
        out += 3;
        if (a->node[1] != 0) {
            out[0] = ':';
            memmove(bytes, &a->node[1], 1);
            out[1] = HIGH_DIGIT(bytes[0]);
            out[2] = LOW_DIGIT(bytes[0]);
            out += 3;
        }
        out[0] = 0;
        break;
    }
}
