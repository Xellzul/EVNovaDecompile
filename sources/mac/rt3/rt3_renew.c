/* rt3_renew.c -- RT3, the license renewal over the network: a request to the registration server over TCP (through an HTTP proxy, as a GET with the license in the query string) or UDP; the mac 1.1.1 librt3_nonag.a(rt3_renew.o) (mac only). Notes: Research/notes/rt3mac.md */
#include "rt3_lib.h"

#define kMaxLine 0x400
#define kTimeoutMs 10000
#define kTCPWait 10
#define kUDPWait 5
#define kOutOfMemory (-7004)
#define kBadParameters (-7001)
#define kTimedOut (-7008)

// MAC: _EncodeParameterSafe i386 0x000d5537 ppc 0x000e07f0
static void EncodeParameterSafe(char *dst, const char *src, ULong size)
{
    unsigned char c;
    ULong hi;
    ULong lo;

    while (size > 3 && (c = *src++) != 0) {
        if ((unsigned char)(c - 0x21) > 0x5e || c == '&' || c == '%' || c == '/') {
            hi = (c >> 4) & 0xf;
            lo = c & 0xf;
            *dst++ = '%';
            *dst++ = hi <= 9 ? hi + '0' : hi + '7';
            *dst++ = lo <= 9 ? lo + '0' : lo + '7';
            size -= 3;
        } else {
            *dst++ = c;
            size--;
        }
    }
    *dst = 0;
}

// MAC: _RenewLicenseTCP i386 0x000d55c8 ppc 0x000e08a0
long RenewLicenseTCP(const char *server, const char *proxy, const char *product, const char *name, const char *copies, const char *code, char *newCode, char *message)
{
    long err = 0;
    long e;
    char *eol = 0;
    MutexList recvQueue;
    MutexList sentQueue;
    char encCopies[0x20];
    char encCode[0x100];
    char encProduct[0x100];
    char encName[0x100];
    NetAddress address;
    char line[kMaxLine];
    NetSocket *stream = 0;
    const char *body;
    NetChunk *chunk;
    char *token;
    long len;

    *newCode = 0;
    *message = 0;
    if (*product == 0 || *name == 0 || *copies == 0 || *code == 0 || StringGetLength(code) <= 11) {
        err = kBadParameters;
        goto cleanup;
    }
    body = copies;
    while (*body != 0 && *body < '0' && *body > '9')
        body++;
    if (*body == 0) {
        err = kBadParameters;
        goto cleanup;
    }
    MemoryClear(&sentQueue, sizeof(sentQueue));
    MemoryClear(&recvQueue, sizeof(recvQueue));
    if (proxy == 0 || *proxy == 0)
        proxy = server;
    e = SimpleResolve(0, proxy, &address);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = QueueCreate(&sentQueue);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = QueueCreate(&recvQueue);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = NetworkQueueFill(&sentQueue, 3, 4, kMaxLine);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    err = kOutOfMemory;
    stream = (NetSocket *)StreamCreate(0, 0, 0, &sentQueue, &sentQueue, &recvQueue, &recvQueue);
    if (stream == 0)
        goto cleanup;
    e = StreamOpenClient(stream, 0, &address, kTimeoutMs);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    EncodeParameterSafe(encProduct, product, sizeof(encProduct));
    EncodeParameterSafe(encName, name, sizeof(encName));
    EncodeParameterSafe(encCopies, copies, sizeof(encCopies));
    EncodeParameterSafe(encCode, code, sizeof(encCode));
    err = kOutOfMemory;
    StringFormatSafe(line, kMaxLine, "GET http://%s/p=%s&n=%s&c=%s&l=%s HTTP/1.1\r\nConnection: Close\r\nPragma: no-cache\r\n\r\n", server, encProduct, encName, encCopies, encCode);
    chunk = (NetChunk *)QueueRemove(&sentQueue);
    if (chunk == 0)
        goto cleanup;
    StringCopy(chunk->data, line);
    chunk->length = StringGetLength(line);
    e = StreamSend(stream, chunk);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    line[0] = 0;
    eol = 0;
    while (StringGetLength(line) + 1 < kMaxLine) {
        chunk = (NetChunk *)NetworkQueueWait(&recvQueue, kTCPWait);
        if (chunk == 0) {
            err = kTimedOut;
            goto cleanup;
        }
        if (chunk->length >= chunk->capacity)
            chunk->length = chunk->capacity - 1;
        chunk->data[chunk->length] = 0;
        StringAppendSafe(line, chunk->data, kMaxLine);
        QueueInsert(&sentQueue, chunk);
        body = StringFindString(line, "\r\n\r\n");
        if (body != 0) {
            body += 4;
            eol = StringFindString(body, "\r\n");
            if (eol != 0)
                break;
        }
    }
    StreamClose(stream, 1);
    if (eol == 0) {
        StringCopy(message, "Server response invalid or incomplete");
    } else if (*body == '<') {
        StringCopy(message, "Renewal blocked by web proxy or firewall");
    } else {
        *eol = 0;
        MemoryCopy(line, body, eol - body + 1);
        eol = 0;
        token = StringTokenize(line, '\t', &eol);
        if (token != 0)
            StringCopySafe(newCode, token, 0x100);
        token = StringTokenize(line, '\t', &eol);
        err = 0;
        if (token != 0) {
            len = StringGetLength(token);
            if (token[0] == '<' && token[len - 1] == '>') {
                token[len - 1] = 0;
                token++;
            }
            StringCopySafe(message, token, 0x100);
        }
    }
    err = 0;
cleanup:
    if (stream != 0)
        StreamDispose(stream);
    QueueEmpty(&recvQueue);
    QueueDispose(&recvQueue);
    QueueEmpty(&sentQueue);
    QueueDispose(&sentQueue);
    return err;
}

// MAC: _RenewLicenseUDP i386 0x000d5b23 ppc 0x000e0ce4
long RenewLicenseUDP(const char *server, const char *product, const char *name, const char *copies, const char *code, char *newCode, char *message)
{
    long err = 0;
    long e;
    char *next;
    MutexList sentQueue;
    MutexList recvQueue;
    NetAddress address;
    char line[kMaxLine];
    const char *p;
    NetSocket *dgram = 0;
    NetPacket *packet;
    char *token;

    *newCode = 0;
    *message = 0;
    if (*product == 0 || *name == 0 || *copies == 0 || *code == 0 || StringGetLength(code) <= 11) {
        err = kBadParameters;
        goto cleanup;
    }
    p = copies;
    while (*p != 0 && *p < '0' && *p > '9')
        p++;
    if (*p == 0) {
        err = kBadParameters;
        goto cleanup;
    }
    MemoryClear(&sentQueue, sizeof(sentQueue));
    MemoryClear(&recvQueue, sizeof(recvQueue));
    e = SimpleResolve(0, server, &address);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = QueueCreate(&sentQueue);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = QueueCreate(&recvQueue);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    e = NetworkQueueFill(&sentQueue, 2, 4, kMaxLine);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    err = kOutOfMemory;
    dgram = DatagramCreate(0, 0, 0, &sentQueue, &sentQueue, &recvQueue, &recvQueue);
    if (dgram == 0)
        goto cleanup;
    e = DatagramOpen(dgram, 0);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    err = kOutOfMemory;
    StringFormatSafe(line, kMaxLine, "%s\t%s\t%s\t%s", product, name, copies, code);
    packet = (NetPacket *)QueueRemove(&sentQueue);
    if (packet == 0)
        goto cleanup;
    AddressDuplicate(&packet->address, &address);
    StringCopy(packet->data, line);
    packet->length = StringGetLength(line);
    e = DatagramSend(dgram, packet);
    if (e != 0) {
        err = e;
        goto cleanup;
    }
    err = kTimedOut;
    packet = (NetPacket *)NetworkQueueWait(&recvQueue, kUDPWait);
    if (packet == 0)
        goto cleanup;
    if (packet->length >= packet->capacity)
        packet->length = packet->capacity - 1;
    packet->data[packet->length] = 0;
    p = packet->data;
    next = 0;
    token = StringTokenize((char *)p, '\t', &next);
    if (token != 0)
        StringCopySafe(newCode, token, 0x100);
    token = StringTokenize((char *)p, '\t', &next);
    if (token != 0)
        StringCopySafe(message, token, 0x100);
    err = 0;
    QueueInsert(&sentQueue, packet);
cleanup:
    if (dgram != 0)
        DatagramDispose(dgram);
    QueueEmpty(&recvQueue);
    QueueDispose(&recvQueue);
    QueueEmpty(&sentQueue);
    QueueDispose(&sentQueue);
    return err;
}
