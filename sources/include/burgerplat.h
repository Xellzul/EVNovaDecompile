/* burgerplat.h -- BurgerLib's platform-layer records; types only. Notes: Research/notes/atplat.md */
#ifndef BURGERPLAT_H
#define BURGERPLAT_H

#include "burgertypes.h"

/* the doubly-linked list */
#ifndef MACBUILD
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
#endif

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
    int32_t period;                                 /* +0x10 */
    ULong armedAt;                               /* +0x14 */
    int32_t request;                                /* +0x18 */
    unsigned char armed;                         /* +0x1c */
    unsigned char quit;                          /* +0x1d */
} TimerThread;

struct PlatformTimer {
    TimerThread *thread;                         /* +0x00 */
    TimerProc proc;                              /* +0x04 */
    void *arg;                                   /* +0x08 */
    int32_t reserved;                               /* +0x0c */
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
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;                               /* negative means top-down */
    unsigned short biPlanes;
    unsigned short biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
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
    uint32_t bmiColors[1];                  /* +0x28 */
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
    int32_t Sample1;                                /* +0x00 */
    int32_t Sample2;                                /* +0x04 */
    int32_t PrevVal;                                /* +0x08 */
    int32_t TableIndex;                             /* +0x0c */
    int32_t LastAmplitude;                          /* +0x10 */
    int32_t LastSlope;                              /* +0x14 */
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
    int32_t failed;                                 /* +0x10 */
} TextBuilder;

struct ITElement;
typedef struct ControlType {
    struct ControlType *next;                    /* +0x00 */
    int32_t (*proc)(void *hwnd, void *dc, struct ITElement *element, void **data, ULong msg,
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
    int32_t h;
    int32_t v;
} ITPoint;

typedef struct ITRect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
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

#ifdef MAC1010
typedef struct ITPort {
    ITPart link;
    CGrafPtr dc;
    void *window;
    unsigned char savedDC;
    int32_t textAlign;                              /* +0x18 */
    int32_t ascent;
    int32_t descent;
    int32_t leading;
    int32_t height;
    int32_t maxWidth;
    Str255 fontFace;
    RgnHandle clip;                              /* +0x130 */
    RGBColor savedFore;
    RGBColor savedBack;
    PenState savedPen;
    short savedFont;
    short savedSize;
    short savedFace;
    short savedMode;
} ITPort;
#else
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
    int32_t penWidth;                               /* +0x24 */
    int32_t fontId;                                 /* +0x28 a stock font number at reset */
    int32_t fontSize;
    ULong fontStyle;
    int32_t textAlign;                                  /* +0x34 the state 0x004ea43e swaps starts here */
    int32_t ascent;                                 /* +0x38 GetTextMetricsA's, at reset */
    int32_t descent;
    int32_t leading;
    int32_t height;
    int32_t maxWidth;
    char fontFace[0x100];                        /* +0x4c the installed family, or "" */
    void *clip;
    int32_t savedIndex;
    int32_t unk154;
    ULong unk158;
} ITPort;
#endif

typedef struct ITDib {
    void *dc;
    void *original;
    void *bits;
    BitmapInfoHeader header;
    RGBQuad colors[256];
    int32_t borrowedBits;
} ITDib;

typedef struct ITPixels {
    ITPart link;
    ULong bytesPerPixel;
    ULong depth;
    ULong flags;
    int32_t locks;
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
#ifdef MAC1010
    ULong baseAddr;
#else
    int32_t baseAddr;
#endif
    ITDib *dib;
    int32_t unk54;
    void *window;
    int32_t drawDC;
#ifdef MAC1010
    ULong info[2];
#else
    ULong info[0x428 / 4];
#endif
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

#ifdef MAC1010
typedef struct ITDevice {
    ITPart link;
    CGrafPtr dc;
    char unk10[0x20 - 0x10];
    int32_t status;
    char unk24[0x28 - 0x24];
    unsigned char docOpen;
    unsigned char pageOpen;
    void *record;                                /* a THPrint; Carbon's a PMPrintSession */
    void *printPort;
} ITDevice;
#else
typedef struct ITDevice {
    ITPart link;
    void *dc;
    char unk10[0x20 - 0x10];
    int32_t status;
    char unk24[0x30 - 0x24];
    void *devMode;
    char unk34[0x70 - 0x34];
    unsigned char printing;
    char unk71[0x7c - 0x71];
} ITDevice;
#endif

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

#ifdef MAC1010
#pragma options align=mac68k
#endif
typedef struct ITElement {
    struct ITElement *next;
    struct ITElement *child;
    int32_t tag;
    char *name;
    struct HashTable *attributes;
    ULong order;
    ULong sourceOffset;
    unsigned char wide;
} ITElement;

typedef struct ITPage {
    struct ITPage *next;
    struct ITPage *link;
    ULong unk08;
    ITElement *element;
    char name[0x100];
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    char title[0x200];
#else
    char title[0x100];
#endif
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
    int32_t index;
    int32_t item;
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

typedef int32_t (*ITControlProc)(void *hwnd, void *dc, ITElement *element, void **data, ULong msg,
                              ULong a, void *b);
typedef int32_t (*ITWindowProc)(struct WindowNode *window, int32_t refcon, int32_t event, void *where,
                             int32_t index, const char *name);

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
} ITControl;

typedef struct WindowNode {
    struct WindowNode *next;
    struct WindowNode *parent;
    ITWindowProc proc;
    int32_t refcon;
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
#ifdef MAC1010
    void *modal;                    /* the DialogPtr of a modal view */
#else
    unsigned char modal;
    unsigned char pad31[3];
#endif
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    short savedKind;
#else
    ULong tracking;
#ifdef MAC1010
    Handle colors;                  /* the window's 'wctb' */
    short savedKind;                /* its windowKind before the toolkit's */
#endif
#endif
    ITImage *image;
    ITImage *buffer;
    ControlType *controlTypes;
    struct ITPreload *preloads;
    ULong preloadTicks;
    char *result;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    ULong tracking;
    char anchor[0x100];
    char path[0x100];
#else
    char path[0x100];
#endif
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
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned short title[0x100];
#else
    char title[0x100];
#endif
} WindowNode;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
void IT_ViewCheckWindows(WindowNode *window);
#define CHECK_WINDOWS(w) IT_ViewCheckWindows(w)
#else
#define CHECK_WINDOWS(w)
#endif

/* what an input event resolved to; `followed' doubles as the window proc's "handled" */
typedef struct ITClick {
    unsigned char followed;
    WindowNode *window;
    struct ITControl *control;
#if defined(MAC1010) && TARGET_API_MAC_CARBON
    unsigned char hasAnchor;
    char anchor[0x100];
#endif
    char link[0x400];
} ITClick;
#ifdef MAC1010
#pragma options align=reset
#endif

/* a condition stored as a byte flag: the exe builds it `mov al,1 / xor al,al`, not setcc */
#define AsByte(c)           ((c) ? (unsigned char)1 : (unsigned char)0)

/* the registration toolkit's license record and usage record; maps in the notes */
typedef struct RT3License {
    char name[256];
    char code[256];
    int32_t copies;
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
#ifdef MAC1010
#define kNetFamilyMask  0xfff
#else
#define kNetFamilyMask  0xff0fLL
#endif

#define kListResolver   1
#define kListDatagram   2
#define kListStream     3
#define kNoSocket       (-1)
#define kOctets         4

typedef struct NetAddress {
    int32_t kind;
    unsigned short family;
    unsigned short port;
    unsigned char node[0xfc];
} NetAddress;

typedef struct NetPacket {
    MutexNode link;
    int32_t unk8[3];
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
#ifdef MAC1010
    ULong flags;
#endif
    MutexList *sentList;
    MutexList *freeList;
    MutexList *recvList;
    MutexList *recvList2;
    unsigned char waitRecv;
    unsigned char waitSend;
    unsigned short pad36;
    int32_t socket;
    NetAddress local;
    NetAddress reset;
    MutexList queue;
    int32_t sendCount;
    int32_t sendBytes;
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
#ifdef MAC1010
    ULong flags;
#endif
    MutexList *sentList;
    MutexList *freeList;
    MutexList *recvList;
    MutexList *recvList2;
    int32_t socket;
    NetAddress local;
    NetAddress remote;
    int32_t connectError;
    MutexList queue;
    int32_t sendCount;
    int32_t sendBytes;
    void *thread;
    void *thread2;
    void *event;
    int32_t accepted;
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
    int32_t unk8[3];
    ULong size;
    ULong length;
    char data[1];
} NetChunk;

typedef ULong (*FrameProc)(int32_t unk0, void *ref, NetChunk *chunk, unsigned char second);

typedef struct NetFramed {
    int32_t unk0;
    void *ref;
    int32_t frameMode;
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
    int32_t unk8[3];
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
#ifdef MAC1010
    ULong flags;
#endif
    MutexList *freeList;
    MutexList *doneList;
    int32_t wanted;
    int32_t pending;
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
    int32_t reserved[2];
} ZState;

#endif
