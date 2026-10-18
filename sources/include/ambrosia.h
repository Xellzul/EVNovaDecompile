/* ambrosia.h -- AmbrosiaTools' shared constants (the at* and it* objects): the file tool's errors and
   info bits, the socket layer's events, and the interface tool's control messages, keys, styles and
   image formats. Types and prototypes stay in burger.h / burgerplat.h. */
#ifndef AMBROSIA_H
#define AMBROSIA_H

/* the file tool (atfilespec*.c) */
#define kNoFile             ((void *)-1)         /* INVALID_HANDLE_VALUE */

#define kFolderShared       -7
#define kFolderDocuments    -6
#define kFolderTemporary    -5
#define kFolderAppData      -4
#define kFolderUserData     -3
#define kFolderApplication  -2

#define kFTErrCancelled     -7007
#define kFTErrBadPath       -7200
#define kFTErrBadName       -7201
#define kFTErrDirNotFound   -7202
#define kFTErrFileNotFound  -7203
#define kFTErrExists        -7204
#define kFTErrBadEntry      -7206
#define kFTErrBusy          -7208
#define kFTErrPermission    -7209
#define kFTErrPosition      -7210
#define kFTErrDiskFull      -7211
#define kFTErrNoVolume      -7212
#define kFTErrTooManyFiles  -7213
#define kFTErrNotAFile      -7214                /* the spec is a file of files */

#define kFTFileSize         0x148                /* sizeof(FTFile) */

/* FTFileInfo.flags; an archive entry keeps the first four as bits of its `flags` */
#define kFTInfoReadable     0
#define kFTInfoWritable     1
#define kFTInfoFolder       2
#define kFTInfoHidden       3
#define kFTInfoArchive      4

#define kEpochYear          1970
#define kDaysPerYear        365
#define kFebruary           2
#define kDefaultEntries     10
#define kMaxEntries         1000
#define ATSwap32(v)         ((long long)Swap32(v))  /* widens: one-operand imul, adc */
#define IsSignature(s, t)   ((s)[0] == (t)[0] && (s)[1] == (t)[1] && (s)[2] == (t)[2] \
                             && (s)[3] == (t)[3])

#define kThreadStack        0x10000              /* every AmbrosiaTools CreateThread */

/* the socket layer (atnet.c, atresolve.c, reggie.c) */
#define kThreadFailed       -7000
#define kSleepMs            5
#define kStateOpen          4

#define kEventCreated       0
#define kEventClosed        1
#define kEventConnected     2
#define kEventDisposed      3
#define kEventNeedBuffers   14
#define kEventPeerClosed    17
#define kEventFailed        18
#define kEventAborted       19

/* Unicode (atcharset.c, atutf162.c, athashtbl.c, convertutf.c) */
#define kAsciiMax           0x7f
#define kBmpMax             0xffff
#define kSurrogateLow       0xdc00
#define kSurrogateHalfMask  0xfffff800
#define kSurrogateLowBit    0x400
#define kConversionOK       0                    /* convertutf.c's results */
#define kSourceExhausted    1
#define kSourcePartial      2
#define kTargetExhausted    3
#define kBudgetSpent        4
#define kSourceIllegal      5
#define kStrictConversion   1                    /* and its flags */
#define kPastNul            2
#define FoldInfo(stride, count) ((count) << 8 | (stride))   /* a FoldRange's stride and count */

/* the interface tool: errors */
#define kNotAWindow         -7002
#define kImageFailed        -7300
#define kStreamOverrun      -7302
#define kWindowFailed       -8400

/* the interface tool: control messages, a control proc's first argument */
#define kMsgCreate          0x00
#define kMsgOpen            0x01
#define kMsgClose           0x02
#define kMsgDispose         0x03
#define kMsgHitTest         0x10
#define kMsgClick           0x11
#define kMsgPaint           0x12
#define kMsgIdle            0x13
#define kMsgWhat            0x15
#define kMsgKey             0x16
#define kMsgPress           0x17
#define kMsgMouse           0x18
#define kMsgEdit            0x20    /* + an edit command */
#define kMsgActivate        0x30
#define kMsgDeactivate      0x31
#define kMsgItems           0x32
#define kMsgFocus           0x33
#define kMsgGetBounds       0x40
#define kMsgBounds          0x41
#define kMsgGetTitle        0x42
#define kMsgTitle           0x43
#define kMsgGetText         0x44
#define kMsgSetText         0x45
#define kMsgChild           0x46
#define kMsgGetValue        0x46
#define kMsgSetValue        0x47
#define kMsgVisible         0x48
#define kMsgShow            0x49
#define kMsgEnabled         0x4a
#define kMsgEnable          0x4b
#define kMsgGetFore         0x4c
#define kMsgFore            0x4d
#define kMsgGetBack         0x4e
#define kMsgBack            0x4f

/* the interface tool: the event the window callback hears when nothing else happens */
#define kEventIdle          4

/* the interface tool: input events */
#define kModCapsLock        0x00000001
#define kModShift           0x00000002
#define kModControl         0x00000004
#define kModAlt             0x00000008
#define kModWindows         0x00000010
#define kModNumLock         0x00000100
#define kModScrollLock      0x00000200
#define kModSwapped         0x00000400
#define kModLeftButton      0x00010000
#define kModRightButton     0x00020000
#define kModMiddleButton    0x00040000
#define kModButtons         (kModLeftButton | kModRightButton | kModMiddleButton)
#define kModStillDown       0x10000000
#define kModRepeat          0x40000000
#define kModMouse           0x80000000UL

#define kKeyReturn          0x91
#define kKeyTab             0x92
#define kKeyEnter           0x95
#define kKeyEscape          0xa0

/* the interface tool: text */
#define kAlignLeft          (-1)
#define kAlignRight         1

#define kStyleBold          0x01
#define kStyleItalic        0x02
#define kStyleUnderline     0x04
#define kStyleTeletype      0x08
#define kStylePre           0x10
#define kStyleLink          0x20
#define kStyleColor         0x40

#define kTagText1           (-3)
#define kTagText            (-1)            /* a text node: its text where the attributes go */
#define kTagElement         1
#define kTagEnd             2

#define kClassName          0x01
#define kClassSpace         0x02

/* the interface tool: images */
#define kIndexedDepth       8
#define kInverseTableSize   0x2000
#define kPixelsKey          0x8889
#define kPixelsInBitmap     0x10
#define kPixelsBitsChanged  0x20
#define kTransparentMask    1
#define kTransparentKey     5

/* the interface tool: limits */
#define kMinLong            ((long)0x80000000)
#define kMaxLong            0x7fffffff
#define kChunkMax           0x32000
#define kPressMs            120
#define kTextMax            0x100

#endif /* AMBROSIA_H */
