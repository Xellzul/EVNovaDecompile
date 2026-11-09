/* rt3_lib.h -- what the library's sources add to rt3_mac.h: the endian swaps, BurgerLib's file tool record and network layer, the proxy and renewal entry points. A declaration added here or in rt3_mac.h can move rt3_common.o's ppc register numbering (uid hash, Research/mac-pads.md). Notes: Research/notes/rt3mac.md */
#ifndef RT3_LIB_H
#define RT3_LIB_H

#include "rt3_mac.h"

/* the license and log files and the network packets are big endian: a swap on the little endian slice only */
#if TARGET_RT_LITTLE_ENDIAN
#define SWAP32(x) ((((x) >> 24) & 0xff) | (((x) >> 8) & 0xff00) | (((x) & 0xff00) << 8) | ((x) << 24))
#define SWAP64(x) ((((ULongLong)(x) >> 56) & 0x00000000000000FFULL) | (((ULongLong)(x) >> 40) & 0x000000000000FF00ULL) | \
                   (((ULongLong)(x) >> 24) & 0x0000000000FF0000ULL) | (((ULongLong)(x) >> 8) & 0x00000000FF000000ULL) | \
                   (((ULongLong)(x) & 0x00000000FF000000ULL) << 8) | (((ULongLong)(x) & 0x0000000000FF0000ULL) << 24) | \
                   (((ULongLong)(x) & 0x000000000000FF00ULL) << 40) | (((ULongLong)(x) & 0x00000000000000FFULL) << 56))
#else
#define SWAP32(x) (x)
#define SWAP64(x) (x)
#endif

/* BurgerLib's file tool information record */
typedef struct FTFileInfo {
    char name[kTextSize];
    unsigned char flags[5];     /* readable, writable, folder, hidden, archive */
    ULong size;
    ULong created;
    ULong modified;
    ULong type;
    ULong creator;
} FTFileInfo;

/* BurgerLib's network layer: the mutex guarded list, a socket address, the endpoint a callback is handed, a stream, and the buffer a stream or a datagram socket queues */
struct MutexNode;

typedef struct MutexList {
    struct MutexNode *head;
    struct MutexNode *tail;
    ULong count;
    void *mutex;
} MutexList;

typedef struct NetAddress {
    char data[0x104];
} NetAddress;

typedef struct NetSocket NetSocket;
typedef struct NetStream NetStream;
struct NetResolver;

typedef struct NetChunk {
    char link[0x14];
    ULong capacity;
    ULong length;
    char data[1];
} NetChunk;

typedef struct NetPacket {
    char link[0x14];
    NetAddress address;
    ULong capacity;
    ULong length;
    char data[1];
} NetPacket;

extern long IndirectSetSize(void **h, ULong size);
extern char *StringFormatSafe(char *out, long size, const char *fmt, ...);
extern char *StringFindString(const char *s, const char *sub);
extern char *StringTokenize(char *s, int delim, char **next);
extern long SimpleResolve(void *unused, const char *host, NetAddress *out);
extern long QueueCreate(void *queue);
extern void QueueDispose(MutexList *list);
extern void QueueEmpty(MutexList *list);
extern void QueueInsert(void *queue, void *element);
extern struct MutexNode *QueueRemove(MutexList *list);
extern long NetworkQueueFill(MutexList *list, unsigned short kind, ULong count, ULong size);
extern struct MutexNode *NetworkQueueWait(MutexList *list, ULong seconds);
extern NetStream *StreamCreate(void *unused, void *ref, void (*proc)(NetSocket *s, void *ref, unsigned short event), MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern long StreamOpenClient(void *stream, NetAddress *local, const NetAddress *remote, ULong timeout);
extern long StreamSend(void *stream, NetChunk *chunk);
extern void StreamClose(NetSocket *sock, unsigned char graceful);
extern void StreamDispose(NetSocket *sock);
extern NetSocket *DatagramCreate(void *unused, void *ref, void (*proc)(NetSocket *s, void *ref, unsigned short event), MutexList *sentList, MutexList *freeList, MutexList *recvList, MutexList *recvList2);
extern long DatagramOpen(void *dgram, NetAddress *address);
extern long DatagramSend(void *dgram, NetPacket *packet);
extern void DatagramDispose(void *dgram);
extern void AddressDuplicate(void *to, const NetAddress *src);
extern void AddressComposeEmpty(NetAddress *a);
extern unsigned char AddressIsEmpty(const NetAddress *a);
extern void AddressComposeTCPIP(void *a, unsigned short port, ULong ip);
extern void AddressDecomposeTCPIP(const void *a, unsigned short *port, ULong *ip);
extern void DatagramClose(void *dgram, long unused);
extern long DatagramControl(void *dgram, void *out, ULong option, ULong value);
extern void NetworkQueueEmpty(MutexList *list);
extern unsigned char NetworkStackGetLoaded(ULong which);
extern long NetworkStackLoad(ULong wanted);
extern struct NetResolver *ResolverCreate(void *unused, void *ref, void (*proc)(NetSocket *s, void *ref, unsigned short event), MutexList *doneList, MutexList *freeList);
extern long ResolverOpen(void *resolver);
extern void ResolverClose(void *resolver);
extern void ResolverDispose(void *resolver);
extern long ResolveName(void *resolver, void *request);
extern unsigned char SystemCheckCallback(void);
extern void SystemSetQuitProc(void (*proc)(void));
extern void SystemIdle(void);
extern ULongLong TimerGetMilliseconds(void);

extern long GetHTTPProxy(char *host, char *user, char *password);
extern long RenewLicenseTCP(const char *server, const char *proxy, const char *product, const char *name,
                            const char *copies, const char *code, char *newCode, char *message);
extern long RenewLicenseUDP(const char *server, const char *product, const char *name, const char *copies,
                            const char *code, char *newCode, char *message);

#endif
