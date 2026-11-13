// COMPILE: -opt level=4,intrinsics -enum min -Cpp_exceptions off
/* cedebug.c -- EV Nova CE: the debug-mode keys and NCB prompts, signed NumToString, the
   hyperspace flash colour (CEFIX only). Notes: Research/cefix-plan.md */
#ifdef CEFIX

#include "macport.h"
#include "gametypes.h"
#include "shipobj.h"
#include "dialogs.h"
#include "game.h"
#include "sound.h"
#include "player.h"
#include "render.h"
#include "burger.h"
#include "cefix.h"

#define kScanNCBTest        0x0a                /* the 9 key */
#define kScanNCBSet         0x0b                /* the 0 key */
#define kNCBExpressionMax   0x40

/* Alt+Home turns debug mode on. In debug mode Alt+9 evaluates a mission bit test
   expression and Alt+0 executes a set expression. Then the key check it replaces. */
unsigned char CE_CheckDebugKeys(short scanCode)
{
    unsigned char len;

    if (!cheatsActive && (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))
            && QuickKeyCheck(kCEScanHome)) {
        PlaySound(beepSnd[0], 1, gSoundVolume, gSoundVolume);
        cheatsActive = 1;
    } else if (cheatsActive && (QuickKeyCheck(kScanLAlt) || QuickKeyCheck(kScanRAlt))) {
        if (QuickKeyCheck(kScanNCBTest)) {
            if (EVTextInputDialog("\pEvaluate NCB Test Expression:", "\p", kNCBExpressionMax)
                    && (len = textInputString[0]) != 0) {
                textInputString[len + 1] = '\0';
                EVNoteAlert(EvalMissionBitTestString((char *)textInputString + 1)
                            ? "\pTrue" : "\pFalse");
            }
        } else if (QuickKeyCheck(kScanNCBSet)) {
            if (EVTextInputDialog("\pExecute NCB Set Expression:", "\p", kNCBExpressionMax)
                    && (len = textInputString[0]) != 0) {
                textInputString[len + 1] = '\0';
                EvalMissionBitSetString((char *)textInputString + 1);
            }
        }
    }
    return QuickKeyCheck(scanCode);
}

/* NumToString's digits, with a minus sign where the original prints a negative value
   as a large unsigned one (debug mode shows some). */
char *CE_NumberToAscii(char *dst, int32_t value, uint32_t flags)
{
    if (value < 0) {
        *dst++ = '-';
        value = -value;
    }
    return NumberToAscii(dst, value, flags);
}

/* The jump, hypergate and wormhole flash, drawn black when the Hyperspace Effects
   preference is off. */
void CE_SetFlashColor(const RGBColor *color)
{
    /* CE bug: CE stores the colour without RGBForeColor's NULL-port test. */
    CurrentGrafPort->foreColor = settings.noHyperGamma != 0 ? *blackColor : *color;
}

#endif /* CEFIX */
