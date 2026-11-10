/* itatsuimac.c -- 1.0.10: ATSUI for Carbon's text box: a laid-out text broken into lines and drawn, a font from a QuickDraw family. Notes: Research/notes/maclibs.md */
#pragma optimization_level 2
#include "burgertypes.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#include <ATSUnicode.h>
#include <Fonts.h>

#define kNoFontErr      -8795

typedef struct ATSULine {
    UniCharArrayOffset start;
    UniCharArrayOffset end;
    ATSUTextMeasurement ascent;
    ATSUTextMeasurement descent;
    ATSUTextMeasurement offset;
} ATSULine;

#define kRotatedNinety  (-90 << 16)

extern OSStatus ATSU_LayoutGetControl(ATSUTextLayout layout, ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value);

OSStatus ATSU_DrawTextBox(ATSUTextLayout layout, const Rect *box, Handle lines)
{
    OSStatus err;
    Handle table;
    UniCharArrayOffset lineStart;
    Fixed x;
    Fixed y;
    Fixed from;
    Fixed to;
    long total;
    ULong i;
    ULong count;
    long offset;
    long ascent;
    long descent;
    ATSULine line;
    ATSTrapezoid bounds;
    UniCharArrayOffset start;
    UniCharCount length;
    UniCharArrayOffset lineEnd;
    Fixed width;
    Fixed rotation;

    err = 0;
    table = 0;
    start = 0;
    length = 0;
    if (box == 0)
        goto done;
    if (lines == 0) {
        table = NewHandle(0);
        err = MemError();
        if (err == 0)
            goto opened;
        goto done;
    } else {
        SetHandleSize(lines, 0);
        err = MemError();
        if (err != 0)
            goto done;
        table = lines;
    }
opened:
    err = ATSUGetTextLocation(layout, 0, 0, &start, &length, 0);
    if (err != 0)
        goto done;
    lineStart = start;
    lineEnd = 0;
    x = box->left << 16;
    y = box->top << 16;
    from = x;
    to = box->right << 16;
    offset = 0;
    total = offset;
    count = 0;
    width = 0;
    rotation = 0;
    err = ATSU_LayoutGetControl(layout, kATSULineRotationTag, sizeof(rotation), &rotation);
    if (err != 0)
        goto done;
    if (rotation != 0) {
        if (rotation != kRotatedNinety)
            goto done;
        x = box->right << 16;
        from = y;
        to = box->bottom << 16;
    }
    err = ATSU_LayoutGetControl(layout, kATSULineWidthTag, sizeof(width), &width);
    if (err != 0)
        goto done;
    if (width == 0)
        width = to - from;
    while (lineStart < start + length) {
        err = ATSUBreakLine(layout, lineStart, width, true, &lineEnd);
        if (err != 0)
            goto done;
        err = ATSUGetGlyphBounds(layout, 0, 0, lineStart, lineEnd - lineStart, kATSUseFractionalOrigins, 1, &bounds, 0);
        if (err != 0)
            goto done;
        if (rotation == 0) {
            ascent = -bounds.upperLeft.y;
            descent = bounds.lowerLeft.y;
        } else {
            ascent = bounds.upperLeft.x;
            descent = -bounds.lowerLeft.x;
        }
        offset = ascent;
        if (total != 0)
            offset = ascent + descent + total;
        total = offset;
        line.start = lineStart;
        line.end = lineEnd;
        line.ascent = ascent;
        line.descent = descent;
        line.offset = offset;
        err = PtrAndHand(&line, table, sizeof(line));
        if (err != 0)
            goto done;
        lineStart = lineEnd;
        count++;
    }
    for (i = 0; i < count; i++) {
        line = (*(ATSULine **)table)[i];
        lineStart = line.start;
        lineEnd = line.end;
        if (kRotatedNinety == rotation)
            x = (box->right << 16) - line.offset;
        if (rotation != kRotatedNinety)
            y = (box->top << 16) + line.offset;
        err = ATSUDrawText(layout, lineStart, lineEnd - lineStart, x, y);
        if (err != 0)
            goto done;
    }
done:
    if (err != 0 && lines != 0)
        SetHandleSize(table, 0);
    if (table != 0 && lines == 0)
        DisposeHandle(table);
    return err;
}

OSStatus ATSU_FontFromFamily(FMFontFamily family, FMFontStyle style, ATSUFontID *font, FMFontStyle *fontStyle)
{
    OSStatus err;

    if (FMGetFontFromFontFamilyInstance != 0)
        return FMGetFontFromFontFamilyInstance(family, style, font, fontStyle);
    if (fontStyle != 0)
        *fontStyle = 0;
    err = ATSUFONDtoFontID(family, style, font);
    if (fontStyle != 0 && (err == 0 || err == kNoFontErr)) {
        FMInput input = { 0, 1, 0, false, 0, { 1, 1 }, { 1, 1 } };
        FMOutput *output;

        input.family = family;
        input.face = style;
        output = FMSwapFont(&input);
        if (output->errNum == 0)
            *fontStyle = style & ~output->curStyle;
        err = 0;
    }
    return err;
}
#endif
