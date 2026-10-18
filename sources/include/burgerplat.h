/* burgerplat.h -- BurgerLib's platform-layer records; types only. Notes: Research/notes/atplat.md */
#ifndef BURGERPLAT_H
#define BURGERPLAT_H

#include "burgertypes.h"

/* the doubly-linked list */
typedef struct ListNode {
    struct ListNode *next;                       /* +0x00 */
    struct ListNode *prev;                       /* +0x04 */
    void *data;                                  /* +0x08 */
    void (*KillProc)(struct ListNode *);         /* +0x0c */
} ListNode;

typedef struct ListRec {
    ListNode *head;                              /* +0x00 */
    ListNode *tail;                              /* +0x04 */
    ULong count;                                 /* +0x08 */
} ListRec;

/* the singly-linked mutex list; each node points back at its list */
typedef struct MutexNode {
    struct MutexNode *next;                      /* +0x00 */
    struct MutexList *owner;                     /* +0x04 */
} MutexNode;

typedef struct MutexList {
    MutexNode *head;                             /* +0x00 */
    MutexNode *tail;                             /* +0x04 */
    ULong count;                                 /* +0x08 */
    void *mutex;                                 /* +0x0c */
} MutexList;

typedef struct PlatformTimer PlatformTimer;
typedef void (*TimerProc)(PlatformTimer *timer, void *arg);

typedef struct TimerThread {
    void *mutex;                                 /* +0x00 */
    void *event;                                 /* +0x04 */
    void *thread;                                /* +0x08 */
    ULong threadId;                              /* +0x0c */
    long period;                                 /* +0x10 */
    ULong armedAt;                               /* +0x14 */
    long request;                                /* +0x18 */
    unsigned char armed;                         /* +0x1c */
    unsigned char quit;                          /* +0x1d */
} TimerThread;

struct PlatformTimer {
    TimerThread *thread;                         /* +0x00 */
    TimerProc proc;                              /* +0x04 */
    void *arg;                                   /* +0x08 */
    long reserved;                               /* +0x0c */
};

/* a file's local time stamp, 12 bytes */
typedef struct TimeDate_t {
    Word32 Year;                                 /* +0x00 */
    Word16 Milliseconds;                         /* +0x04 */
    Word8 Month;                                 /* +0x06 1-12 */
    Word8 Day;                                   /* +0x07 1-31 */
    Word8 DayOfWeek;                             /* +0x08 0-6 */
    Word8 Hour;                                  /* +0x09 */
    Word8 Minute;                                /* +0x0a */
    Word8 Second;                                /* +0x0b */
} TimeDate_t;

/* BurgerLib's directory walker, win32 flavour */
typedef struct DirectorySearch_t {
    Word32 FileSize;                             /* +0x00 */
    TimeDate_t Create;                           /* +0x04 */
    TimeDate_t Modify;                           /* +0x10 */
    char Dir;                                    /* +0x1c */
    char System;                                 /* +0x1d */
    char Hidden;                                 /* +0x1e */
    char Locked;                                 /* +0x1f */
    char Name[256];                              /* +0x20 */
    void *FindHandle;                            /* +0x120 */
    Word HandleOk;                               /* +0x124 */
    char MyFindT[320];                           /* +0x128 */
} DirectorySearch_t;

/* Win32's BITMAPINFOHEADER and BITMAPINFO (one RGBQUAD) */
typedef struct BitmapInfoHeader {
    unsigned long biSize;
    long biWidth;
    long biHeight;                               /* negative means top-down */
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long biCompression;
    unsigned long biSizeImage;
    long biXPelsPerMeter;
    long biYPelsPerMeter;
    unsigned long biClrUsed;
    unsigned long biClrImportant;
} BitmapInfoHeader;

/* Win32's RGBQUAD */
typedef struct RGBQuad {
    unsigned char blue;                          /* +0 */
    unsigned char green;                         /* +1 */
    unsigned char red;                           /* +2 */
    unsigned char reserved;                      /* +3 */
} RGBQuad;

typedef struct BitmapInfo {
    BitmapInfoHeader bmiHeader;                  /* +0x00 */
    unsigned long bmiColors[1];                  /* +0x28 */
} BitmapInfo;

/* BurgerLib's parsed-audio descriptor */
typedef struct AudioDesc {
    Word8                  *SoundPtr;                /* +0x00 */
    ULong                  SoundLength;              /* +0x04 */
    ULong                  LoopStart;                /* +0x08 */
    ULong                  LoopEnd;                  /* +0x0c */
    void                   (*Proc)(void *);          /* +0x10 */
    void                   *Data;                    /* +0x14 */
    ULong                  DataType;                 /* +0x18 */
    ULong                  SampleRate;               /* +0x1c */
    Word8                  *CompPtr;                 /* +0x20 */
    ULong                  Extra1;                   /* +0x24 */
    ULong                  Extra2;                   /* +0x28 */
    ULong                  Extra3;                   /* +0x2c */
    ULong                  SampleCount;              /* +0x30 */
    float                  TimeInSeconds;            /* +0x34 */
} AudioDesc;

/* a streamed song's decoder state (SnDigitalSong) */
struct DigitalMusicReadState_t;
typedef Word32 (*DigitalMusicReadProcPtr)(void *Data, Word8 *Buffer, Word32 Length);
typedef Word32 (*DigitalMusicDecodeProcPtr)(struct DigitalMusicReadState_t *State, Word Command,
                                          Word8 *Buffer, Word32 Length);

typedef struct DigitalMusicReadState_t {
    DigitalMusicReadProcPtr ReadProc;            /* +0x00 pulls raw bytes from the stream */
    void *CallBackParm;                          /* +0x04 */
    DigitalMusicDecodeProcPtr CodecProc;         /* +0x08 the decoder for DataType */
    void *CompressStatePtr;                      /* +0x0c the decoder's work buffer */
    Word32 FileOffset;                           /* +0x10 where the samples start in the file */
    Word32 SoundLength;                          /* +0x14 */
    Word32 BytesPlayed;                          /* +0x18 */
    Word32 DataType;                             /* +0x1c */
} DigitalMusicReadState_t;

/* the MACE expanders' running state (PkMACE) */
typedef struct MACEState_t {
    long Sample1;                                /* +0x00 */
    long Sample2;                                /* +0x04 */
    long PrevVal;                                /* +0x08 */
    long TableIndex;                             /* +0x0c */
    long LastAmplitude;                          /* +0x10 */
    long LastSlope;                              /* +0x14 */
} MACEState_t;

/* one MS ADPCM block to expand (PkADPCM) */
typedef struct ADPCMUnpackState_t {
    Word8 *SrcPtr;                               /* +0x00 */
    Word32 SrcLength;                            /* +0x04 */
    Word Channels;                               /* +0x08 */
    Word BlockSize;                              /* +0x0c */
    Word SamplesPerBlock;                        /* +0x10 */
    short *OutputPtr;                            /* +0x14 */
} ADPCMUnpackState_t;

/* a handle is a record pointer; the record's first word is the block */
struct MemBlock;

typedef struct MemRecord {
    void *ptr;                                   /* +0x00 */
    ULong guard;                                 /* +0x04 */
    ULong size;                                  /* +0x08 */
    unsigned char locked;                        /* +0x0c */
    unsigned char used;                          /* +0x0d */
    struct MemBlock *block;                      /* +0x10 */
} MemRecord;

typedef struct MemBlock {
    ULong count;                                 /* +0x00 */
    ULong reserved[2];                           /* +0x04 */
    struct MemBlock *next;                       /* +0x0c */
    MemRecord records[1];                        /* +0x10 */
} MemBlock;

#define kRecordGuard      0xdeadbeefUL

/* a text buffer that grows its handle */
typedef struct TextBuilder {
    char **handle;                               /* +0x00, a Handle */
    ULong length;                                /* +0x04 */
    ULong capacity;                             /* +0x08 */
    ULong extent;                                /* +0x0c */
    long failed;                                 /* +0x10 */
} TextBuilder;

struct ITElement;
typedef struct ControlType {
    struct ControlType *next;                    /* +0x00 */
    long (*proc)(void *hwnd, void *dc, struct ITElement *element, void **data, ULong msg,
                 ULong a, void *b);              /* +0x04 an ITControlProc */
    char name[0x100];                            /* +0x08 */
} ControlType;

/* one row of the character-set registry's per-code tables */
typedef struct CharEntry {
    unsigned short unicode;                      /* +0 */
    unsigned char iso;                           /* +2 ISO-8859-1 */
    unsigned char win;                           /* +3 Windows-1252 */
    unsigned char mac;                           /* +4 MacRoman */
    char entity[7];                              /* +5 */
} CharEntry;

typedef struct WinEntry {
    unsigned short unicode;
    unsigned char win;
    unsigned char reserved;
} WinEntry;

typedef struct ITPoint {
    long h;
    long v;
} ITPoint;

typedef struct ITRect {
    long left;
    long top;
    long right;
    long bottom;
} ITRect;

typedef struct ITColor {
    unsigned char alpha;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} ITColor;

typedef struct ITPalette {
    ULong count;
    ITColor entry[256];
} ITPalette;

typedef struct ITStream {
    void **data;
    ULong size;
    ULong pos;
} ITStream;

/* what an image loader reports besides the image: how it is transparent, and its key */
typedef struct ITTransparent {
    ULong mode;
    ITColor key;                                 /* alpha: the blend level, or an indexed key (0xff none) */
} ITTransparent;

typedef struct ITBackground {
    ULong format;                                /* 'JPEG', ... */
    double gamma;
    ITColor color;
    unsigned char hasColor;                      /* 'bKGD' present */
    unsigned char interlaced;
    unsigned char pad16;
} ITBackground;

/* a QuickDraw ColorTable as the PICT and ppat readers unpack it */
typedef struct PictColorSpec {
    unsigned short value;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
} PictColorSpec;

typedef struct PictColorTable {
    ULong seed;
    unsigned short flags;
    unsigned short size;                         /* entries - 1 */
    PictColorSpec entry[256];
} PictColorTable;

struct ITImage;

typedef struct ITPart {
    struct ITImage *owner;
    struct ITImage *owner2;
    struct ITImage *self;
} ITPart;

typedef struct ITPort {
    ITPart link;
    void *dc;
    void *window;
    unsigned char savedDC;
    unsigned char pad15[2];
    unsigned char ownPen;
    unsigned char ownFont;
    unsigned char ownBrush;
    unsigned char pad1a[2];
    const unsigned char *pattern;                /* +0x1c the 8 x 8 fill pattern */
    const unsigned char *penPattern;             /* +0x20 NULL or the solid one: a plain pen */
    long penWidth;                               /* +0x24 */
    long fontId;                                 /* +0x28 a stock font number at reset */
    long fontSize;
    ULong fontStyle;
    long textAlign;                                  /* +0x34 the state 0x004ea43e swaps starts here */
    long ascent;                                 /* +0x38 GetTextMetricsA's, at reset */
    long descent;
    long leading;
    long height;
    long maxWidth;
    char fontFace[0x100];                        /* +0x4c the installed family, or "" */
    void *clip;
    long savedIndex;
    long unk154;
    ULong unk158;
} ITPort;

typedef struct ITDib {
    void *dc;
    void *original;
    void *bits;
    BitmapInfoHeader header;
    RGBQuad colors[256];
    long borrowedBits;
} ITDib;

typedef struct ITPixels {
    ITPart link;
    ULong bytesPerPixel;
    ULong depth;
    ULong flags;
    long locks;
    ULong key;
    unsigned char formatFlag;
    unsigned char bits[4];
    unsigned char shift[4];
    unsigned char mask[4];
    unsigned char pad2d;
    unsigned short scale[4];
    unsigned char pad36[2];
    ITRect bounds;
    ULong rowBytes;
    long baseAddr;
    ITDib *dib;
    long unk54;
    void *window;
    long drawDC;
    ULong info[0x428 / 4];
    ITPalette palette;
    unsigned char inverse[0x2000];
} ITPixels;

typedef struct ITPixelInfo {
    ULong bytesPerPixel;
    ULong depth;
    ULong rowBytes;
    ULong otherRowBytes;
    ITRect bounds;
    void *bits;
    void *otherBits;
    ITDib *dib;
    struct ITImage *image;
    void *dc;
    struct ITImage *portImage;
    ITPalette *palette;
    unsigned char *inverse;
    ULong key;
    unsigned char channelBits[4];
    unsigned char shift[4];
    unsigned char mask[4];
    unsigned short scale[4];
    unsigned char formatFlag;
    unsigned char pad59[3];
} ITPixelInfo;

typedef struct ITDevice {
    ITPart link;
    void *dc;
    char unk10[0x20 - 0x10];
    long status;
    char unk24[0x30 - 0x24];
    void *devMode;
    char unk34[0x70 - 0x34];
    unsigned char printing;
    char unk71[0x7c - 0x71];
} ITDevice;

typedef struct ITImage {
    struct ITImage *portSelf;
    struct ITImage *pixelsSelf;
    struct ITImage *deviceSelf;
    ITPort *port;
    ITPixels *pixels;
    ITDevice *device;
} ITImage;

/* the interface tool's window (0x2a8) and control (0x2c) records; map: Research/notes/ittoolkit.md */
struct WindowNode;
struct ITControl;

typedef struct ITElement {
    struct ITElement *next;
    struct ITElement *child;
    long tag;
    char *name;
    struct HashTable *attributes;
    ULong order;
    ULong sourceOffset;
    unsigned char wide;
    unsigned char pad1d[3];
} ITElement;

typedef struct ITPage {
    struct ITPage *next;
    struct ITPage *link;
    ULong unk08;
    ITElement *element;
    char name[0x100];
    char title[0x100];
} ITPage;

typedef struct ITInsert {
    struct ITInsert *next;
    struct ITControlList *list;
    ITElement *element;
} ITInsert;

typedef struct ITControlList {
    ITElement *root;
    struct ITControl *first;
    ITInsert *inserts;
} ITControlList;

typedef struct ITPageTable {
    ITElement *root;
    ULong count;
    ULong buckets;
    ITPage *first;
    ITPage *bucket[32];
} ITPageTable;

typedef struct ITPreload {
    struct ITPreload *next;
    ITImage *image;
    ITImage *mask;
    ITTransparent transparent;
    ULong stamp;
    unsigned char pending;
    char name[1];
} ITPreload;

typedef struct ITSource {
    struct FolderSpec **folder;
    char *file;
    ITElement *root;
    ITElement *interfaceElement;    /* the INTERFACE element shown */
} ITSource;

typedef struct ITFocus {
    struct WindowNode *root;
    struct WindowNode *window;
    struct ITControl *control;
    long index;
    long item;
} ITFocus;

typedef struct ITInputEvent {
    ITPoint screen;
    ITPoint where;
    void *hwnd;
    ULong modifiers;
} ITInputEvent;

typedef struct ITKeyEvent {
    unsigned char ch;
    unsigned char key;
    unsigned char vk;
    unsigned char pad03;
    ULong code;
    ULong modifiers;
} ITKeyEvent;

typedef long (*ITControlProc)(void *hwnd, void *dc, ITElement *element, void **data, ULong msg,
                              ULong a, void *b);
typedef long (*ITWindowProc)(struct WindowNode *window, long refcon, long event, void *where,
                             long index, const char *name);

typedef struct ITControl {
    struct ITControl *next;
    ITControlList *list;
    void **items;
    ITElement *element;
    ITRect bounds;
    ITControlProc proc;
    void *data;
    unsigned char ownsElement;
    unsigned char active;
    unsigned char pad2a[2];
} ITControl;

typedef struct WindowNode {
    struct WindowNode *next;
    struct WindowNode *parent;
    ITWindowProc proc;
    long refcon;
    ITRect bounds;
    unsigned char dirty;
    unsigned char unk21;
    unsigned char closing;
    unsigned char titled;
    unsigned char unbuffered;
    unsigned char visible;
    unsigned char enabled;
    unsigned char pad27;
    void *dc;
    void *hwnd;
    unsigned char modal;
    unsigned char pad31[3];
    ULong tracking;
    ITImage *image;
    ITImage *buffer;
    ControlType *controlTypes;
    struct ITPreload *preloads;
    ULong preloadTicks;
    char *result;
    char path[0x100];
    ITSource source;
    struct HashTable *variables;
    ITPageTable *pages;
    ITControlList *controls;
    ITPage *page;
    ITFocus *focus;
    ITFocus focusRecord;
    void **focusables;
    ITColor fore;
    ITColor back;
    ITRect frame;                   /* the INTERFACE's RECT, or WIDTH x HEIGHT */
    ITImage *backdrop;
    char title[0x100];
} WindowNode;

/* what an input event resolved to; `followed' doubles as the window proc's "handled" */
typedef struct ITClick {
    unsigned char followed;
    unsigned char pad01[3];
    WindowNode *window;
    struct ITControl *control;
    char link[0x400];
} ITClick;

/* a condition stored as a byte flag: the exe builds it `mov al,1 / xor al,al`, not setcc */
#define AsByte(c)           ((c) ? (unsigned char)1 : (unsigned char)0)

/* the registration toolkit's license record and usage record; maps in the notes */
typedef struct RT3License {
    char name[256];
    char code[256];
    long copies;
    ULong machine;
    ULong level;
    ULong user;
    ULong reserved[2];
} RT3License;

typedef struct RT3LogEntry {
    char product[256];
    ULong firstLaunch;
    ULong secondsUsed;
    ULong launches;
    ULong closeStamp;
    ULong saveStamp;
    ULong reserved[2];
} RT3LogEntry;

typedef struct FTFileInfo {
    char name[256];
    unsigned char flags[5];     /* readable, writable, folder, hidden, archive */
    ULong size;                 /* bytes, or a folder's entry count */
    ULong created;
    ULong modified;
    ULong type;
    ULong creator;
} FTFileInfo;

/* the file manager's extension to Mac type/creator map */
typedef struct FileTypeMap {
    ULong type;
    ULong creator;
    ULong extension;                             /* left-justified, NUL-padded */
    ULong preferred;
} FileTypeMap;

#define kNetFamilyNone  0
#define kNetFamilyInet  2
#define kNetFamilyIPX   0x100
#define kNetFamilyMask  0xff0fLL

#define kListResolver   1
#define kListDatagram   2
#define kListStream     3
#define kNoSocket       (-1)
#define kOctets         4

typedef struct NetAddress {
    long kind;
    unsigned short family;
    unsigned short port;
    unsigned char node[0xfc];
} NetAddress;

typedef struct NetPacket {
    MutexNode link;
    long unk8[3];
    NetAddress from;
    ULong capacity;
    ULong length;
    char data[1];
} NetPacket;

struct NetSocket;
typedef void (*NetProc)(struct NetSocket *s, void *ref, unsigned short event);

typedef struct NetSocket {
    struct NetSocket *next;
    unsigned short kind;
    unsigned short unk6;
    unsigned short state;
    unsigned short padA;
    ULong lock[4];
    NetProc proc;
    void *ref;
    MutexList *sentList;
    MutexList *freeList;
    MutexList *recvList;
    MutexList *recvList2;
    unsigned char waitRecv;
    unsigned char waitSend;
    unsigned short pad36;
    long socket;
    NetAddress local;
    NetAddress reset;
    MutexList queue;
    long sendCount;
    long sendBytes;
    void *thread;
    void *thread2;
    unsigned char quit;
    unsigned char done;
    unsigned char done2;
    unsigned char reportReset;
} NetSocket;

typedef struct NetStream {
    struct NetStream *next;
    unsigned short kind;
    unsigned short unk6;
    unsigned short state;
    unsigned short padA;
    ULong lock[4];
    NetProc proc;
    void *ref;
    MutexList *sentList;
    MutexList *freeList;
    MutexList *recvList;
    MutexList *recvList2;
    long socket;
    NetAddress local;
    NetAddress remote;
    long connectError;
    MutexList queue;
    long sendCount;
    long sendBytes;
    void *thread;
    void *thread2;
    void *event;
    long accepted;
    NetAddress acceptedPeer;
    unsigned char shutdownSend;
    unsigned char quit;
    unsigned char done;
    unsigned char done2;
    unsigned char waitRecv;
    unsigned char waitSend;
    unsigned short pad376;
} NetStream;

typedef struct NetChunk {
    MutexNode link;
    long unk8[3];
    ULong size;
    ULong length;
    char data[1];
} NetChunk;

typedef ULong (*FrameProc)(long unk0, void *ref, NetChunk *chunk, unsigned char second);

typedef struct NetFramed {
    long unk0;
    void *ref;
    long frameMode;
    NetProc proc;
    ULong flags;
    NetChunk *partial;
    NetChunk *partial2;
    MutexList queue;
    MutexList queue2;
    MutexList *freeList;
    MutexList *outList;
    MutexList *outList2;
} NetFramed;

typedef struct NetLookup {
    MutexNode link;
    long unk8[3];
    NetAddress address;
    char name[0x100];
} NetLookup;

typedef struct NetResolver {
    struct NetResolver *next;
    unsigned short kind;
    unsigned short unk6;
    unsigned short state;
    unsigned short padA;
    ULong lock[4];
    NetProc proc;
    void *ref;
    MutexList *freeList;
    MutexList *doneList;
    long wanted;
    long pending;
    unsigned char waiting;
    unsigned char pad35[3];
    MutexList requests;
    void *thread;
    NetLookup *current;
    unsigned char quit;
    unsigned char done;
} NetResolver;

typedef struct HostEnt {
    char *name;
    char **aliases;
    short addrType;
    short length;
    char **addrList;
} HostEnt;

typedef struct ZState {
    ULong magic;
    struct z_stream_s *strm;
    long reserved[2];
} ZState;

#endif
