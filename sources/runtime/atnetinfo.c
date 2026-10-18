// COMPILE: -opt level=2,intrinsics,space -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (mac AmbrosiaTools.framework) is one library on its own compiler: its parts inline memset (rep stos in the exe), so ,intrinsics; -enum min -align 8 are the library's struct layout; these functions lose under REGGIE's -g; build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atnetinfo.c -- the network layer's socket accessors (kind, state, event proc, the four packet lists), the last socket error, and dotted-address text. Notes: Research/notes/atnetinfo.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"
#include "win32.h"

char DAT_00577a80[256] = "*";                      /* 0x00577a80 nothing reads it */
char DAT_00577b80[256] = "*";                      /* 0x00577b80 nothing reads it */

/* EndpointGetElem (a socket's list kind). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f339b
unsigned short EndpointGetElem(void *ref)
{
    long err;
    NetSocket *s;

    err = 0;
    s = (NetSocket *)ref;
    return s->kind;
}

/* EndpointGetStack (a socket's +0x06 word). STATE: MATCH 100%, exe 6 / obj 6, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f33aa
unsigned short EndpointGetStack(void *ref)
{
    long err;
    NetSocket *s;

    err = 0;
    s = (NetSocket *)ref;
    return s->unk6;
}

/* EndpointGetStatus (a socket's state as its owner sees it). STATE: MATCH 100%, exe 11 / obj 11,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f33b9
unsigned short EndpointGetStatus(void *ref)
{
    long err;
    NetSocket *s;

    err = 0;
    s = (NetSocket *)ref;
    return BufferedStreamStatus(ref, s->state);
}

/* EndpointGetCallback (a socket's event proc and reference, under the lock). STATE: MATCH 100%, exe
   26 / obj 26, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f33d1
void EndpointGetCallback(void *handle, NetProc *proc, void **ref)
{
    char token[16];
    NetSocket *s;
    long err;

    err = 0;
    s = (NetSocket *)handle;
    AtomicEnterCritical(token);
    *proc = s->proc;
    *ref = s->ref;
    AtomicLeaveCritical(token);
}

/* _EndpointGetError (the socket's last error). STATE: MATCH 100%, exe 5 / obj 5, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x004f340c
long _EndpointGetError(NetSocket *sock)
{
    long err;
    NetStream *s;

    err = 0;
    s = (NetStream *)sock;
    err = WSAGetLastError();
    return err;
}

#define kOctetDigits    3
#define kPortDigits     5
#define kMaxOctet       0xff
#define kMaxPort        0xffff

/* Step past the previous field and its separator, and read the next octet's digits.
   BUG: the no-digit test can never be true, so an empty field reads as 0 */
#define ReadOctet() \
    text = text + n + 1; \
    n = 0; \
    i = i + 1U; \
    if (*text < '0' && *text > '9') \
        valid = 0; \
    else \
        while ((c = text[n]) >= '0' && c <= '9') { \
            value[i] = value[i] * 10 + (c - '0'); \
            n++; \
        }

/* _AddressFromString (a dotted IPv4 address with an optional ":port", " port" or ",port"; the
   address is cleared when the text is not one). STATE: MATCH 100%, exe 236 / obj 236,
   byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f341a
unsigned char _AddressFromString(NetAddress *address, const char *text)
{
    long err;
    unsigned char valid;
    ULong value[kOctets + 1];
    unsigned char ip[kOctets];
    long i;
    ULong n;
    unsigned short port;
    char c;
    char sep;
    long j;

    err = 0;
    valid = 1;
    sep = 0;
    value[0] = value[1] = value[2] = value[3] = value[4] = 0;
    n = i = err = -1;
    if (valid) {
        ReadOctet();
        if (n > kOctetDigits || text[n] != '.' || value[i] > kMaxOctet)
            valid = 0;
    }
    if (valid) {
        ReadOctet();
        if (n > kOctetDigits || text[n] != '.' || value[i] > kMaxOctet)
            valid = 0;
    }
    if (valid) {
        ReadOctet();
        if (n > kOctetDigits || text[n] != '.' || value[i] > kMaxOctet)
            valid = 0;
    }
    if (valid) {
        ReadOctet();
        if (n > kOctetDigits || value[i] > kMaxOctet)
            valid = 0;
        c = text[n];
        if (c != 0 && c != ':' && c != ',' && c != ' ')
            valid = 0;
    }
    if (valid) {
        c = text[n];
        if (c == ':' || c == ',' || c == ' ') {
            text = text + n + 1;
            n = 0;
            if (*text < '0' && *text > '9')
                valid = 0;
            else
                while ((c = text[n]) >= '0' && c <= '9') {
                    j = i;
                    j++;
                    value[j] = value[j] * 10 + (c - '0');
                    n++;
                }
            if (n > kPortDigits || value[++i] > kMaxPort)
                valid = 0;
            if (text[n] != 0)
                valid = 0;
        }
    }
    if (!valid) {
        AddressComposeEmpty(address);
    } else {
        ip[0] = value[0];
        ip[1] = value[1];
        ip[2] = value[2];
        ip[3] = value[3];
        port = value[4];
        AddressComposeTCPIP(address, (long long)Swap16(port), *(ULong *)ip);
    }
    return valid;
}

/* DatagramGetQueues (a UDP socket's four lists, each wanted one filled). STATE: MATCH 100%, exe 32
   / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f36c3
void DatagramGetQueues(void *ref, MutexList **sentList, MutexList **freeList,
                  MutexList **recvList, MutexList **recvList2)
{
    NetSocket *s;
    long err;

    s = (NetSocket *)ref;
    err = 0;
    if (sentList != 0)
        *sentList = s->sentList;
    if (freeList != 0)
        *freeList = s->freeList;
    if (recvList != 0)
        *recvList = s->recvList;
    if (recvList2 != 0)
        *recvList2 = s->recvList2;
}

/* StreamGetQueues (a TCP socket's four lists, each wanted one filled). STATE: MATCH 100%, exe 32
   / obj 32, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x004f3709
void StreamGetQueues(void *ref, MutexList **sentList, MutexList **freeList,
                  MutexList **recvList, MutexList **recvList2)
{
    NetStream *s;
    long err;

    s = (NetStream *)ref;
    err = 0;
    if (sentList != 0)
        *sentList = s->sentList;
    if (freeList != 0)
        *freeList = s->freeList;
    if (recvList != 0)
        *recvList = s->recvList;
    if (recvList2 != 0)
        *recvList2 = s->recvList2;
}
