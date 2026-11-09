/* the opcodes below 0xa2 (Inside Macintosh: Imaging With QuickDraw, A-22) */
AT_STATIC PictOpcode gDT_PictOpTable[kPictLastTableOp + 1] = {
    PICT_OP(0, NULL),                           /* 0x00 NOP */
    PICT_OP(kOpWordSize, NULL),                 /* 0x01 Clip */
    PICT_OP(8, NULL),                           /* 0x02 BkPat */
    PICT_OP(2, NULL),                           /* 0x03 TxFont */
    PICT_OP(1, NULL),                           /* 0x04 TxFace */
    PICT_OP(2, NULL),                           /* 0x05 TxMode */
    PICT_OP(4, NULL),                           /* 0x06 SpExtra */
    PICT_OP(4, NULL),                           /* 0x07 PnSize */
    PICT_OP(2, NULL),                           /* 0x08 PnMode */
    PICT_OP(8, NULL),                           /* 0x09 PnPat */
    PICT_OP(8, NULL),                           /* 0x0a FillPat */
    PICT_OP(4, NULL),                           /* 0x0b OvSize */
    PICT_OP(4, NULL),                           /* 0x0c Origin */
    PICT_OP(2, NULL),                           /* 0x0d TxSize */
    PICT_OP(4, NULL),                           /* 0x0e FgColor */
    PICT_OP(4, NULL),                           /* 0x0f BkColor */
    PICT_OP(8, NULL),                           /* 0x10 TxRatio */
    PICT_OP(1, NULL),                           /* 0x11 VersionOp */
    PICT_OP(kOpHandler, _PICT_SkipPixPat),      /* 0x12 BkPixPat */
    PICT_OP(kOpHandler, _PICT_SkipPixPat),      /* 0x13 PnPixPat */
    PICT_OP(kOpHandler, _PICT_SkipPixPat),      /* 0x14 FillPixPat */
    PICT_OP(2, NULL),                           /* 0x15 PnLocHFrac */
    PICT_OP(2, NULL),                           /* 0x16 ChExtra */
    PICT_OP(0, NULL),                           /* 0x17 reserved */
    PICT_OP(0, NULL),
    PICT_OP(0, NULL),
    PICT_OP(6, NULL),                           /* 0x1a RGBFgCol */
    PICT_OP(6, NULL),                           /* 0x1b RGBBkCol */
    PICT_OP(0, NULL),                           /* 0x1c HiliteMode */
    PICT_OP(6, NULL),                           /* 0x1d HiliteColor */
    PICT_OP(0, NULL),                           /* 0x1e DefHilite */
    PICT_OP(6, NULL),                           /* 0x1f OpColor */
    PICT_DRAW_OP(8),                            /* 0x20 Line */
    PICT_DRAW_OP(4),                            /* 0x21 LineFrom */
    PICT_DRAW_OP(6),                            /* 0x22 ShortLine */
    PICT_DRAW_OP(2),                            /* 0x23 ShortLineFrom */
    PICT_DRAW_OP(kOpWordLength),                /* 0x24 reserved */
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(kOpLongText),                  /* 0x28 LongText */
    PICT_DRAW_OP(kOpDHText),                    /* 0x29 DHText */
    PICT_DRAW_OP(kOpDHText),                    /* 0x2a DVText */
    PICT_DRAW_OP(kOpDHDVText),                  /* 0x2b DHDVText */
    PICT_OP(kOpLongText, NULL),                 /* 0x2c fontName */
    PICT_OP(10, NULL),                          /* 0x2d lineJustify */
    PICT_OP(8, NULL),                           /* 0x2e glyphState */
    PICT_OP(kOpWordLength, NULL),               /* 0x2f reserved */
    PICT_DRAW_OP(8),                            /* 0x30 frameRect */
    PICT_DRAW_OP(8),                            /* 0x31 paintRect */
    PICT_OP(8, NULL),                           /* 0x32 eraseRect */
    PICT_DRAW_OP(8),                            /* 0x33 invertRect */
    PICT_DRAW_OP(8),                            /* 0x34 fillRect */
    PICT_DRAW_OP(8),                            /* 0x35 reserved */
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(0),                            /* 0x38 frameSameRect */
    PICT_DRAW_OP(0),                            /* 0x39 paintSameRect */
    PICT_DRAW_OP(0),                            /* 0x3a eraseSameRect */
    PICT_DRAW_OP(0),                            /* 0x3b invertSameRect */
    PICT_DRAW_OP(0),                            /* 0x3c fillSameRect */
    PICT_DRAW_OP(0),                            /* 0x3d reserved */
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(8),                            /* 0x40 frameRRect */
    PICT_DRAW_OP(8),                            /* 0x41 paintRRect */
    PICT_DRAW_OP(8),                            /* 0x42 eraseRRect */
    PICT_DRAW_OP(8),                            /* 0x43 invertRRect */
    PICT_DRAW_OP(8),                            /* 0x44 fillRRect */
    PICT_DRAW_OP(8),                            /* 0x45 reserved */
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(0),                            /* 0x48 frameSameRRect */
    PICT_DRAW_OP(0),                            /* 0x49 paintSameRRect */
    PICT_DRAW_OP(0),                            /* 0x4a eraseSameRRect */
    PICT_DRAW_OP(0),                            /* 0x4b invertSameRRect */
    PICT_DRAW_OP(0),                            /* 0x4c fillSameRRect */
    PICT_DRAW_OP(0),                            /* 0x4d reserved */
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(8),                            /* 0x50 frameOval */
    PICT_DRAW_OP(8),                            /* 0x51 paintOval */
    PICT_DRAW_OP(8),                            /* 0x52 eraseOval */
    PICT_DRAW_OP(8),                            /* 0x53 invertOval */
    PICT_DRAW_OP(8),                            /* 0x54 fillOval */
    PICT_DRAW_OP(8),                            /* 0x55 reserved */
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(8),
    PICT_DRAW_OP(0),                            /* 0x58 frameSameOval */
    PICT_DRAW_OP(0),                            /* 0x59 paintSameOval */
    PICT_DRAW_OP(0),                            /* 0x5a eraseSameOval */
    PICT_DRAW_OP(0),                            /* 0x5b invertSameOval */
    PICT_DRAW_OP(0),                            /* 0x5c fillSameOval */
    PICT_DRAW_OP(0),                            /* 0x5d reserved */
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(12),                           /* 0x60 frameArc */
    PICT_DRAW_OP(12),                           /* 0x61 paintArc */
    PICT_DRAW_OP(12),                           /* 0x62 eraseArc */
    PICT_DRAW_OP(12),                           /* 0x63 invertArc */
    PICT_DRAW_OP(12),                           /* 0x64 fillArc */
    PICT_DRAW_OP(12),                           /* 0x65 reserved */
    PICT_DRAW_OP(12),
    PICT_DRAW_OP(12),
    PICT_DRAW_OP(4),                            /* 0x68 frameSameArc */
    PICT_DRAW_OP(4),                            /* 0x69 paintSameArc */
    PICT_DRAW_OP(4),                            /* 0x6a eraseSameArc */
    PICT_DRAW_OP(4),                            /* 0x6b invertSameArc */
    PICT_DRAW_OP(4),                            /* 0x6c fillSameArc */
    PICT_DRAW_OP(4),                            /* 0x6d reserved */
    PICT_DRAW_OP(4),
    PICT_DRAW_OP(4),
    PICT_DRAW_OP(kOpWordSize),                  /* 0x70 framePoly */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x71 paintPoly */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x72 erasePoly */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x73 invertPoly */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x74 fillPoly */
    PICT_DRAW_OP(kOpWordLength),                /* 0x75 reserved */
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(0),                            /* 0x78 frameSamePoly */
    PICT_DRAW_OP(0),                            /* 0x79 paintSamePoly */
    PICT_DRAW_OP(0),                            /* 0x7a eraseSamePoly */
    PICT_DRAW_OP(0),                            /* 0x7b invertSamePoly */
    PICT_DRAW_OP(0),                            /* 0x7c fillSamePoly */
    PICT_DRAW_OP(0),                            /* 0x7d reserved */
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(kOpWordSize),                  /* 0x80 frameRgn */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x81 paintRgn */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x82 eraseRgn */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x83 invertRgn */
    PICT_DRAW_OP(kOpWordSize),                  /* 0x84 fillRgn */
    PICT_DRAW_OP(kOpWordLength),                /* 0x85 reserved */
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(kOpWordLength),
    PICT_DRAW_OP(0),                            /* 0x88 frameSameRgn */
    PICT_DRAW_OP(0),                            /* 0x89 paintSameRgn */
    PICT_DRAW_OP(0),                            /* 0x8a eraseSameRgn */
    PICT_DRAW_OP(0),                            /* 0x8b invertSameRgn */
    PICT_DRAW_OP(0),                            /* 0x8c fillSameRgn */
    PICT_DRAW_OP(0),                            /* 0x8d reserved */
    PICT_DRAW_OP(0),
    PICT_DRAW_OP(0),
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x90 BitsRect */
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x91 BitsRgn */
    PICT_OP(kOpWordLength, NULL),               /* 0x92 reserved */
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x98 PackBitsRect */
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x99 PackBitsRgn */
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x9a DirectBitsRect */
    PICT_OP(kOpHandler, _PICT_HandleBits),      /* 0x9b DirectBitsRgn */
    PICT_OP(kOpWordLength, NULL),               /* 0x9c reserved */
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(kOpWordLength, NULL),
    PICT_OP(2, NULL),                           /* 0xa0 ShortComment */
    PICT_OP(kOpLongComment, NULL)               /* 0xa1 LongComment */
};
