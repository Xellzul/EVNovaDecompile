/* itatsuportmac.c -- 1.0.10: ATSUI attributes by tag, a style from a QuickDraw port, text drawn or measured on a CGContext; the `bl` to ATSU_FontFromFamily, backward with a `nop`, is an object seam. Notes: Research/notes/maclibs.md */
#pragma optimization_level 2
#include "burgertypes.h"
#include "burgerplat.h"
#if TARGET_API_MAC_CARBON
#include <ATSUnicode.h>
#include <Fonts.h>
#include <Script.h>
#include <FixMath.h>

extern OSStatus ATSU_FontFromFamily(FMFontFamily family, FMFontStyle style, ATSUFontID *font, FMFontStyle *fontStyle);
extern void *_PlatformFindBundleSymbol(CFStringRef bundleName, CFStringRef symbolName);

#define kATSUNotSet     -8801
#define kRenderingOptionsTag    0x115
#define kDirectionTag   3
#define kCGContextTag   0x7fff
#define kOneFixed       65536.0f
#define kNoPosition     -1.0f
#define kMinWidth       0x8000
#define kWidthTooWide   32767.0f
#define kWidthTooNarrow 32768.0f

typedef struct CGPoint {
    float x;
    float y;
} CGPoint;

static CGPoint (*sCGContextGetTextPosition)(void *context) = NULL;
static void (*sCGContextSetTextPosition)(void *context, float x, float y) = NULL;
static void (*sCGContextSaveGState)(void *context) = NULL;
static void (*sCGContextRestoreGState)(void *context) = NULL;
static OSStatus (*sSyncCGContextOriginWithPort)(void *context, GrafPtr port) = NULL;

OSStatus ATSU_StyleSetByte(ATSUStyle style, unsigned char value, ATSUAttributeTag tag)
{
    ByteCount size = sizeof(value);
    ATSUAttributeValuePtr pointer = &value;

    return ATSUSetAttributes(style, 1, &tag, &size, &pointer);
}

OSStatus ATSU_StyleSetFixed(ATSUStyle style, Fixed value, ATSUAttributeTag tag)
{
    ByteCount size = sizeof(value);
    ATSUAttributeValuePtr pointer = &value;

    return ATSUSetAttributes(style, 1, &tag, &size, &pointer);
}

OSStatus ATSU_StyleSetFont(ATSUStyle style, ATSUFontID value, ATSUAttributeTag tag)
{
    ByteCount size = sizeof(value);
    ATSUAttributeValuePtr pointer = &value;

    return ATSUSetAttributes(style, 1, &tag, &size, &pointer);
}

OSStatus ATSU_StyleSetFace(ATSUStyle style, short face)
{
    ATSUAttributeTag tags[5] = { kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag, kATSUQDCondensedTag, kATSUQDExtendedTag };
    ByteCount sizes[5] = { sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean) };
    ATSUAttributeValuePtr values[5];
    Boolean isBold = (face & bold) != 0;
    Boolean isItalic = (face & italic) != 0;
    Boolean isUnderlined = (face & underline) != 0;
    Boolean isCondensed = (face & condense) != 0;
    Boolean isExtended = (face & extend) != 0;

    values[0] = &isBold;
    values[1] = &isItalic;
    values[2] = &isUnderlined;
    values[3] = &isCondensed;
    values[4] = &isExtended;
    return ATSUSetAttributes(style, 5, tags, sizes, values);
}

OSStatus ATSU_StyleSetColor(ATSUStyle style, RGBColor color)
{
    ATSUAttributeTag tag = kATSUColorTag;
    ByteCount size = sizeof(color);
    ATSUAttributeValuePtr pointer = &color;

    return ATSUSetAttributes(style, 1, &tag, &size, &pointer);
}

OSStatus ATSU_LayoutSetFixed(ATSUTextLayout layout, Fixed value, ATSUAttributeTag tag)
{
    ByteCount size = sizeof(value);
    ATSUAttributeValuePtr pointer = &value;

    return ATSUSetLayoutControls(layout, 1, &tag, &size, &pointer);
}

OSStatus ATSU_LayoutSetOptions(ATSUTextLayout layout, ATSLineLayoutOptions value)
{
    ATSUAttributeTag tag = kATSULineLayoutOptionsTag;
    ByteCount size = sizeof(value);
    ATSUAttributeValuePtr pointer = &value;

    return ATSUSetLayoutControls(layout, 1, &tag, &size, &pointer);
}

OSStatus ATSU_LayoutGetControl(ATSUTextLayout layout, ATSUAttributeTag tag, ByteCount size, ATSUAttributeValuePtr value)
{
    OSStatus err = ATSUGetLayoutControl(layout, tag, size, value, 0);

    return err == kATSUNotSet ? 0 : err;
}

OSStatus ATSU_StyleFromPort(ATSUStyle style, GrafPtr port, Boolean clear)
{
    ATSUAttributeTag tags[8] = { kATSUFontTag, kATSUSizeTag, kATSUQDBoldfaceTag, kATSUQDItalicTag, kATSUQDUnderlineTag,
                                 kATSUQDCondensedTag, kATSUQDExtendedTag, kATSUColorTag };
    ByteCount sizes[8] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(Boolean), sizeof(Boolean), sizeof(Boolean),
                           sizeof(Boolean), sizeof(Boolean), sizeof(RGBColor) };
    ATSUAttributeValuePtr values[8];
    RGBColor color;
    GrafPtr saved;
    OSStatus err;
    short font;
    short size;
    short face;
    ATSUFontID id;
    Fixed fixedSize;
    short fontStyle;
    Boolean bold;
    Boolean italic;
    Boolean underlined;
    Boolean condensed;
    Boolean extended;

    GetPort(&saved);
    if (port == 0)
        port = saved;
    else
        SetPort(port);
    font = GetPortTextFont(port);
    size = GetPortTextSize(port);
    face = GetPortTextFace(port);
    err = ATSU_FontFromFamily(font, face, &id, &fontStyle);
    if (err == 0) {
        face &= ~fontStyle;
        bold = face & 1;
        italic = (face >> 1) & 1;
        underlined = (face >> 2) & 1;
        condensed = (face >> 5) & 1;
        extended = (face >> 6) & 1;
        if (size == 0)
            size = (unsigned short)GetScriptVariable(FontToScript(font), smScriptPrefFondSize);
        fixedSize = Long2Fix(size);
        GetForeColor(&color);
        if (clear)
            err = ATSUClearStyle(style);
        if (err == 0) {
            values[0] = &id;
            values[1] = &fixedSize;
            values[2] = &bold;
            values[3] = &italic;
            values[4] = &underlined;
            values[5] = &condensed;
            values[6] = &extended;
            values[7] = &color;
            err = ATSUSetAttributes(style, 8, tags, sizes, values);
        }
    }
    SetPort(saved);
    return err;
}

#define kApplicationServices "ApplicationServices.framework"
#define LOAD_CG_SYMBOL(var, bundle, name) \
    if (var == 0) \
        *(void **)&var = _PlatformFindBundleSymbol(CFSTR(bundle), CFSTR(name))
#define LOAD_CG_SYMBOLS() \
    LOAD_CG_SYMBOL(sCGContextGetTextPosition, kApplicationServices, "CGContextGetTextPosition"); \
    LOAD_CG_SYMBOL(sCGContextSetTextPosition, kApplicationServices, "CGContextSetTextPosition"); \
    LOAD_CG_SYMBOL(sCGContextSaveGState, kApplicationServices, "CGContextSaveGState"); \
    LOAD_CG_SYMBOL(sCGContextRestoreGState, kApplicationServices, "CGContextRestoreGState"); \
    LOAD_CG_SYMBOL(sSyncCGContextOriginWithPort, "Carbon.framework", "SyncCGContextOriginWithPort"); \
    if (sCGContextGetTextPosition == 0 || sCGContextSetTextPosition == 0 || sCGContextSaveGState == 0 \
        || sCGContextRestoreGState == 0 || sSyncCGContextOriginWithPort == 0) \
        return -1

OSStatus ATSU_DrawTextAtPen(const UniChar *text, UniCharArrayOffset start, UniCharCount length, void *context);

OSStatus ATSU_DrawText(const UniChar *text, UniCharArrayOffset start, UniCharCount length)
{
    return ATSU_DrawTextAtPen(text, start, length, 0);
}

OSStatus ATSU_DrawTextAtPen(const UniChar *text, UniCharArrayOffset start, UniCharCount length, void *context)
{
    OSStatus err;
    unsigned char saved;
    ATSUStyle style;
    ATSUTextLayout layout;
    GrafPtr port;
    ATSUAttributeTag tag;
    ByteCount size;
    ATSUAttributeValuePtr value;
    Point pen;
    Boolean leftToRight;
    CGPoint position;
    ATSUCaret caret;
    ATSUCaret caret2;

    saved = 0;
    style = 0;
    layout = 0;
    if (context != 0) {
        LOAD_CG_SYMBOLS();
    }
    err = ATSUCreateStyle(&style);
    if (err != 0)
        goto done;
    err = ATSU_StyleFromPort(style, 0, 0);
    if (err != 0)
        goto done;
    err = ATSU_StyleSetByte(style, 1, kRenderingOptionsTag);
    if (err != 0)
        goto done;
    err = ATSUCreateTextLayoutWithTextPtr(text, start, length, start + length, 1, &length, &style, &layout);
    if (err != 0)
        goto done;
    err = ATSUSetTransientFontMatching(layout, 1);
    if (context != 0) {
        value = &context;
        tag = kCGContextTag;
        size = sizeof(context);
        sCGContextSaveGState(context);
        saved = 1;
        err = ATSUSetLayoutControls(layout, 1, &tag, &size, &value);
        if (err != 0)
            goto done;
        GetPort(&port);
        err = sSyncCGContextOriginWithPort(context, port);
        if (err != 0)
            goto done;
        position = sCGContextGetTextPosition(context);
    }
    if (err != 0)
        goto done;
    GetPen(&pen);
    err = ATSU_LayoutGetControl(layout, kDirectionTag, sizeof(leftToRight), &leftToRight);
    if (err != 0)
        goto done;
    err = ATSUOffsetToPosition(layout, leftToRight ? start : start + length, false, &caret, 0, 0);
    if (err != 0)
        goto done;
    err = ATSUDrawText(layout, kATSUFromTextBeginning, kATSUToTextEnd, context != 0 ? position.x : kNoPosition, context != 0 ? position.y : kNoPosition);
    if (err != 0)
        goto done;
    MoveTo(pen.h + Fix2Long(caret.fX), pen.v + Fix2Long(caret.fY));
    if (context != 0)
        sCGContextSetTextPosition(context, position.x + caret.fX / kOneFixed, position.y + caret.fY / kOneFixed);
done:
    if (layout != 0)
        ATSUDisposeTextLayout(layout);
    if (style != 0)
        ATSUDisposeStyle(style);
    if (context != 0 && saved)
        sCGContextRestoreGState(context);
    return err;
}

OSStatus ATSU_MeasureText(const UniChar *text, UniCharArrayOffset start, UniCharCount length, void *context, float *width);

OSStatus ATSU_Measure(const UniChar *text, UniCharArrayOffset start, UniCharCount length, float *width)
{
    return ATSU_MeasureText(text, start, length, 0, width);
}

OSStatus ATSU_MeasureText(const UniChar *text, UniCharArrayOffset start, UniCharCount length, void *context, float *width)
{
    OSStatus err;
    unsigned char saved;
    ATSUStyle style;
    ATSUTextLayout layout;
    GrafPtr port;
    ATSUAttributeTag tag;
    ByteCount size;
    ATSUAttributeValuePtr value;
    Point pen;
    Boolean leftToRight;
    ATSUCaret caret;
    ATSUCaret caret2;
    int32_t n;

    saved = 0;
    style = 0;
    layout = 0;
    *width = 0.0f;
    if (context != 0) {
        LOAD_CG_SYMBOLS();
    }
    err = ATSUCreateStyle(&style);
    if (err != 0)
        goto done;
    err = ATSU_StyleFromPort(style, 0, 0);
    if (err != 0)
        goto done;
    err = ATSU_StyleSetByte(style, 1, kRenderingOptionsTag);
    if (err != 0)
        goto done;
    err = ATSUCreateTextLayoutWithTextPtr(text, start, length, start + length, 1, &length, &style, &layout);
    if (err != 0)
        goto done;
    err = ATSUSetTransientFontMatching(layout, 1);
    if (context != 0) {
        value = &context;
        tag = kCGContextTag;
        size = sizeof(context);
        sCGContextSaveGState(context);
        saved = 1;
        err = ATSUSetLayoutControls(layout, 1, &tag, &size, &value);
        if (err != 0)
            goto done;
        GetPort(&port);
        err = sSyncCGContextOriginWithPort(context, port);
        if (err != 0)
            goto done;
    }
    if (err != 0)
        goto done;
    GetPen(&pen);
    err = ATSU_LayoutGetControl(layout, kDirectionTag, sizeof(leftToRight), &leftToRight);
    if (err != 0)
        goto done;
    err = ATSUOffsetToPosition(layout, leftToRight ? start : start + length, false, &caret, 0, 0);
    if (err != 0)
        goto done;
    n = Fix2Long(caret.fX);
    if (n > 0x7fff)
        *width = kWidthTooWide;
    else if (n < kMinWidth)    /* BUG: 0x8000, not -0x8000, so every width that fits lands here */
        *width = kWidthTooNarrow;
    else
        *width = caret.fX / kOneFixed;
done:
    if (layout != 0)
        ATSUDisposeTextLayout(layout);
    if (style != 0)
        ATSUDisposeStyle(style);
    if (context != 0 && saved)
        sCGContextRestoreGState(context);
    return err;
}
#endif
