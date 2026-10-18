// COMPILE: -opt level=2,speed -enum min -align 8 -Cpp_exceptions off
// COMPILER: mwccsym2 2.4.7 build 335
// COMPILE-WHY: AmbrosiaTools (the mac build imports it as AmbrosiaTools.framework) on its own compiler as one library: the sound tool is its speed part (16-byte aligned, where 335's level 2 is packed) and calls memset at constant sizes, so no ,intrinsics; -enum min -align 8 are the library's struct layout (jpeglib.h's structs in itjpeg.c, a double at 8); build 335 has no -proc; Research/runtime-compiler-ceiling.md
/* atadpcm.c -- IMA ADPCM: Jack Jansen's public-domain coder that packs sounds held in memory, and the decoder that resamples and mixes them. Notes: Research/notes/atadpcm.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerchan.h"

#define kMaxStepIndex 88

static int indexTable[16] = {                    /* 0x005794e8 */
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int stepsizeTable[kMaxStepIndex + 1] = {  /* 0x00579528 */
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const double DAT_00579690[4] = {          /* 0x00579690, unreferenced */
    0.0, -1.0, 1.0, 2147483648.0
};

/* adpcm_coder. STATE: MATCH 100%, exe 110 / obj 110, byte-identical yes.
   OPEN: none */
// FUNCTION: EVNOVA 0x00507e20
void adpcm_coder(short indata[], char outdata[], int len,
                 struct adpcm_state *state)
{
    short *inp;
    signed char *outp;
    int val;
    int sign;
    int delta;
    int diff;
    int step;
    int valpred;
    int vpdiff;
    int index;
    int outputbuffer;
    int bufferstep;

    outp = (signed char *)outdata;
    inp = indata;

    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 1;

    for (; len > 0; len--) {
        val = *inp++;

        diff = val - valpred;
        sign = (diff < 0) ? 8 : 0;
        if (sign)
            diff = -diff;

        delta = 0;
        vpdiff = step >> 3;

        if (diff >= step) {
            delta = 4;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            delta |= 2;
            diff -= step;
            vpdiff += step;
        }
        step >>= 1;
        if (diff >= step) {
            delta |= 1;
            vpdiff += step;
        }

        if (sign)
            valpred -= vpdiff;
        else
            valpred += vpdiff;

        if (valpred > 32767)
            valpred = 32767;
        else if (valpred < -32768)
            valpred = -32768;

        delta |= sign;

        index += indexTable[delta];
        if (index < 0)
            index = 0;
        if (index > kMaxStepIndex)
            index = kMaxStepIndex;
        step = stepsizeTable[index];

        if (bufferstep)
            outputbuffer = (delta << 4) & 0xf0;
        else
            *outp++ = (delta & 0x0f) | outputbuffer;
        bufferstep = !bufferstep;
    }

    if (!bufferstep)
        *outp++ = outputbuffer;

    state->valprev = valpred;
    state->index = index;
}

#define kNibblesPerWord   8
#define kFirstNibbleShift 28
#define kRateShift        12
#define kFracOne          0x100
#define kLevelShift       7

/* Decode a voice at a 20.12 step, add it into *out (stereo 16-bit, frames
   long) with linear interpolation, and advance. out 0 decodes only.
   0xff once the voice has run out. */
/* ADPCM_Mixer. STATE: MATCH 100%, exe 233 / obj 233, byte-identical yes. OPEN: none */
// FUNCTION: EVNOVA 0x00507f90
unsigned char ADPCM_Mixer(ULong *data, short **out, ULong frames,
                           ULong step, AdpcmVoice *voice,
                           unsigned short left, unsigned short right)
{
    long index;
    ULong *wp;
    long vpdiff;
    long stepsize;
    long valprev;
    long shift;
    ULong remaining;
    ULong pos;
    long lastL;
    ULong accum;
    ULong lastAccum;
    long lastR;
    unsigned char done;
    short *dst;
    ULong word;
    ULong delta;
    long r;
    long inc;
    long s;
    long t;

    accum = lastAccum = 0;
    done = 0;
    valprev = voice->valprev;
    index = voice->index;
    pos = voice->pos;
    lastL = voice->lastLeft;
    lastR = voice->lastRight;
    if (out != 0)
        dst = *out;
    else
        dst = 0;

    wp = data + pos / kNibblesPerWord;
    stepsize = stepsizeTable[index];
    shift = kFirstNibbleShift - (pos & (kNibblesPerWord - 1)) * 4;
    word = *wp++;
    word = Swap32(word);
    remaining = voice->total;
    remaining -= pos;

    while (remaining != 0) {
        if (shift < 0) {
            word = *wp++;
            word = Swap32(word);
            shift = kFirstNibbleShift;
        }
        delta = (word >> shift) & 0xf;
        shift -= 4;
        remaining--;
        pos++;

        index += indexTable[delta];
        r = ((delta & 7) << 4) | 8;
        if (index < 0)
            index = 0;
        else if (index > kMaxStepIndex)
            index = kMaxStepIndex;
        vpdiff = (r * stepsize) >> 6;
        if (delta & 8) {
            valprev -= vpdiff;
            if (valprev < -32768)
                valprev = -32768;
        } else {
            valprev += vpdiff;
            if (valprev > 32767)
                valprev = 32767;
        }

        accum += step;
        delta = (accum >> kRateShift) - (lastAccum >> kRateShift);
        if (delta >= 1) {
            lastAccum = accum;
            vpdiff = left * valprev >> kLevelShift;
            r = (right * valprev) >> kLevelShift;
            if (dst != 0) {
                inc = kFracOne / delta;
                for (stepsize = inc; stepsize <= kFracOne; stepsize += inc) {
                    s = (((vpdiff - lastL) * stepsize) >> 8) + lastL + dst[0];
                    if (s < -32768)
                        s = -32768;
                    else if (s > 32767)
                        s = 32767;
                    dst[0] = s;
                    t = (((r - lastR) * stepsize) >> 8) + lastR + dst[1];
                    if (t < -32768)
                        t = -32768;
                    else if (t > 32767)
                        t = 32767;
                    dst[1] = t;
                    dst += 2;
                    if (--frames == 0)
                        goto finished;
                }
            } else {
                inc = kFracOne / delta;
                for (stepsize = inc; stepsize <= kFracOne; stepsize += inc) {
                    if (--frames == 0)
                        goto finished;
                }
            }
            lastL = vpdiff;
            lastR = r;
        }
        stepsize = stepsizeTable[index];
    }

finished:
    if (remaining == 0)
        done = 0xff;
    voice->valprev = valprev;
    voice->index = index;
    voice->pos = pos;
    voice->lastLeft = lastL;
    voice->lastRight = lastR;
    if (out != 0)
        *out = dst;
    return done;
}
