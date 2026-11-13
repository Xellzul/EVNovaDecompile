/* reggiesession.h -- the version-check session objects. Notes: Research/notes/reggiesession.md */

#ifndef REGGIESESSION_H
#define REGGIESESSION_H

#include "burgerplat.h"
#include "burger.h"

#define kSessionNameLen  0x100
#define kSessionEntry    0x350
#ifdef MAC1010
extern unsigned char (*gReggieSharedProc)(NetSocket *s);
extern unsigned char (*gReggiePacketProc)(NetSocket *s, const char *data, ULong length,
                               const NetAddress *from);
extern unsigned char (*gReggiePrejoinProc)(NetSocket *s, const NetAddress *local,
                                  const NetAddress *from);
extern unsigned char (*gReggieSendFailedProc)(NetSocket *s, const NetAddress *from);
extern int32_t (*gNT_MatchedProc)(NetSocket *s, const NetAddress *peer);
extern int32_t (*gNT_InvitedProc)(NetSocket *s, const NetAddress *peer);
#define Net16(v)      (v)
#define WideNet32(v)  (v)
#else
#define Net16(v)      Swap16(v)
#define WideNet32(v)  ((long long)Swap32(v))
#endif
#define kEntryKindInsert 1
#define kEntryKindLookup 2
#define kEntryKindMessages 3
#define kEntryKindNamed  4
#define kEntryKindDirect 5
#define kEntryKindMatch  6
#define kEntryKindInvite 7
#define kModeShared      1
#define kModeOwnUdp      2
#define kModeTcp         3
#define kModeNoExtras    kModeTcp

typedef struct LookupRecord {
    struct HashTable *table;                     /* +0x000 */
    uint32_t askedAt;                       /* +0x004, 0 answered, 1 never asked */
} LookupRecord;

typedef struct SessionEntry {
    struct SessionEntry *next;                   /* +0x000 */
    ULong kind;                                  /* +0x004 */
    uint32_t slot;                          /* +0x008 */
    uint32_t counter;                       /* +0x00c */
    ULong insertMode;                            /* +0x010 */
    char idText[kSessionNameLen];                /* +0x014 "%lu/%lu" */
    void *request;                               /* +0x114 */
    uint32_t requestedAt;                   /* +0x118 */
    uint32_t cancelledAt;                   /* +0x11c */
    unsigned char retries;                       /* +0x120 */
    LookupRecord *records;                       /* +0x124 */
    uint32_t wanted;                        /* +0x128 */
    uint32_t recordCount;                   /* +0x12c */
    uint32_t answered;                      /* +0x130 */
    uint32_t updates;                       /* +0x134 */
    uint32_t unk138;                        /* +0x138 */
    uint32_t lastAsked;                     /* +0x13c */
    uint32_t received;                      /* +0x140 */
    union {
        struct {
            char name[kSessionNameLen];          /* +0x144 */
            void *product;                       /* +0x244 */
        } named;
        struct {
            void **pending;                      /* +0x144 */
            void **received;                     /* +0x148 */
            struct HashTable *table;             /* +0x14c */
            struct HashTable *permissions;       /* +0x150 */
            char lastCxid[kSessionNameLen];      /* +0x154 */
        } messages;
        struct {
            struct HashTable *query;             /* +0x144 */
            char kindName[kSessionNameLen];      /* +0x148 */
            unsigned char notified;              /* +0x248 */
            unsigned char finished;              /* +0x249 */
        } lookup;
        struct {
            uint32_t natType;               /* +0x144 */
            uint32_t startedAt;             /* +0x148 */
            struct NetSocket *socket;            /* +0x14c */
            NetAddress peer;                     /* +0x150 */
            unsigned char portMapped;            /* +0x254 */
            unsigned char tcp;                   /* +0x255 */
            unsigned short localPort;            /* +0x256 */
            MutexList queue;                     /* +0x258 */
            MutexList queue2;                    /* +0x268 */
        } direct;
        struct {
            unsigned char incoming;              /* +0x144 */
            unsigned char tcp;                   /* +0x145 */
            unsigned short pad146;
            NetAddress local;                    /* +0x148 */
            NetAddress remote;                   /* +0x24c */
        } invite;
    } u;
} SessionEntry;

struct SessionContext;
typedef void (*ReggieProc)(struct SessionContext *ctx, void *ref, uint32_t slot,
                           uint32_t event);

typedef struct SessionContext {
    struct SessionContext *next;                 /* +0x000 */
    ULong state;                                 /* +0x004 */
    ULong mode;                                  /* +0x008 */
    struct NetSocket *socket;                    /* +0x00c */
    NetAddress server;                           /* +0x010 */
    NetAddress probe;                            /* +0x114 */
    NetAddress publicAddress;                    /* +0x218 */
    unsigned char portMapped;                    /* +0x31c */
    unsigned char pad31d[3];
    uint32_t natType;                       /* +0x320 */
    ZState deflater;                             /* +0x324 */
    ZState inflater;                             /* +0x334 */
    unsigned char lengthFrames;                  /* +0x344 */
    unsigned char inCallback;                    /* +0x345 */
    unsigned short pad346;
    ReggieProc proc;                             /* +0x348 */
    void *procRef;                               /* +0x34c */
    char kind[kSessionNameLen];                  /* +0x350 */
    char user[kSessionNameLen];                  /* +0x450 */
    char password[kSessionNameLen];              /* +0x550 */
    MutexList *freeList;                         /* +0x650 */
    MutexList *recvList;                         /* +0x654 */
    int32_t unk658[8];                              /* +0x658 */
    uint32_t receivedAt;                    /* +0x678 */
    uint32_t sentAt;                        /* +0x67c */
    uint32_t stateAt;                       /* +0x680 */
    uint32_t stateTicks;                    /* +0x684 */
    unsigned char helloRetried;                  /* +0x688 */
    unsigned char streamLost;                    /* +0x689 */
    unsigned short pad68a;
    uint32_t resets;                        /* +0x68c */
    char instance[kSessionNameLen];              /* +0x690 */
    char motd[kSessionNameLen];                  /* +0x790 */
    char motdUrl[kSessionNameLen];               /* +0x890 */
    int32_t clockOffset;                            /* +0x990 */
    uint32_t hostSource;                    /* +0x994, a host id number */
    void *fallback;                              /* +0x998 */
    uint32_t lastSlot;                      /* +0x99c */
    SessionEntry *entries;                       /* +0x9a0 */
} SessionContext;

/* a product the REGGIE server reported; payload is its $-key table */
typedef struct GlobalEntry {
    struct GlobalEntry *next;                    /* +0x000 */
    char name[kSessionNameLen];                  /* +0x004 */
    struct HashTable *payload;                   /* +0x104 */
} GlobalEntry;

extern GlobalEntry *gVersionList;

extern int32_t ReggieBeginVersionDirect(const char *name);
AT_STATIC int32_t _ReggieChannelPacket(SessionContext *ctx, SessionEntry *entry);
AT_STATIC int32_t ReggieChannelOpen(void *ref, uint32_t *slot);
AT_STATIC int32_t ReggieTickle(void *ref);

/* the named entry whose folded name is NAME's */
static inline SessionEntry *FindNamed(SessionContext *ctx, const char *name)
{
    char copied[kSessionNameLen];
    char wanted[kSessionNameLen];
    char candidate[kSessionNameLen];
    SessionEntry *entry;
    int32_t err;

    err = 0;
    entry = 0;
    StringCopySafe(copied, name, kSessionNameLen);
    _ReggieFixProductName(wanted, copied);
    for (entry = ctx->entries; entry != 0; entry = entry->next)
        if (entry->kind == kEntryKindNamed) {
            _ReggieFixProductName(candidate, entry->u.named.name);
            if (StringCompare(wanted, candidate) == 0)
                return entry;
        }
    return 0;
}

/* a request's HOSTID field, the host id as text */
static inline int32_t AddHostID(struct HashTable *request, ULong host)
{
    char hostText[kSessionNameLen];

    return HashInsert(request, "HOSTID", StringFromNumber(hostText, host));
}

#endif
