// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);noieeefp, the band's float model; runtime P band, one line for BurgerLib and its Ogg/Vorbis (user ruling 2026-09-27: -proc pentium4 for the one big band, no per-file flags); its one body is a call-free leaf the band census cannot class, 16-aligned between burgermace.c's and framing.c's P bodies; -lang c++ -bool off: BurgerLib is C++ (the archive's mangled statics, COM calls as virtual calls) (Research/runtime-compiler-ceiling.md)
/* burgermsadpcm.c -- BurgerLib's Microsoft ADPCM block expander (PkADPCM). Notes: Research/notes/burgermsadpcm.md */

#include "burgertypes.h"
#include "burger.h"
#include "burgerplat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define kNibbleValues       16                   /* gaiP4 holds four tables back to back */
#define kCoef1              32
#define kCoef2              39
#define kPredictors         7
#define kMonoHeader         7
#define kMinDelta           16
#define kSampleMax          32767
#define kSampleMin          (-32768)

static const long gaiP4[46] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230,
    0, 1, 2, 3, 4, 5, 6, 7,
    -8, -7, -6, -5, -4, -3, -2, -1,
    256, 512, 0, 192, 240, 460, 392,
    0, -256, 0, 64, 0, -208, -232
};

/* One channel's decoder state (the modern BurgerLib's ADPCMState_t). */
typedef struct ADPCMState_t {
    long sample1;                                /* the older of the two */
    long sample2;
    long Coef1;
    long Coef2;
    long index;                                  /* the adaptive step */
} ADPCMState_t;

/* One nibble through one channel's predictor; the decoded sample. */
static inline long ADPCMDecode(ADPCMState_t *State, Word DeltaCode)
{
    long Index;
    long NewIndex;
    long Predict;
    long Sample;

    DeltaCode = DeltaCode & 0xf;
    Index = State->index;
    NewIndex = (gaiP4[DeltaCode] * Index) >> 8;
    if (NewIndex < kMinDelta)
        NewIndex = kMinDelta;
    State->index = NewIndex;
    Index = gaiP4[kNibbleValues + DeltaCode] * Index;
    Predict = State->sample1 * State->Coef2;
    Sample = State->sample2;
    State->sample1 = Sample;
    Sample = ((Sample * State->Coef1) + Predict) >> 8;
    Index = Index + Sample;
    if (Index > kSampleMax)
        Index = kSampleMax;
    else if (Index <= kSampleMin)
        Index = kSampleMin;
    State->sample2 = Index;
    return Index;
}

/* ADPCMDecodeBlock (BurgerLib PkADPCM). STATE: MATCH 100.0%, exe 327 / obj 327,
   byte-identical yes (1208 bytes). OPEN: none */
// FUNCTION: EVNOVA 0x00530b90
Word ADPCMDecodeBlock(ADPCMUnpackState_t *StatePtr)
{
    Word32 Temp;
    Word32 samplesThisBlock;
    Word8 *bytePtr;
    short *OutputPtr;
    Word32 Count;
    ADPCMState_t state[2];
    ADPCMState_t MonoState;

    Temp = StatePtr->BlockSize;
    if (StatePtr->SrcLength < Temp)
        Temp = StatePtr->SrcLength;
    StatePtr->SrcLength -= Temp;
    bytePtr = StatePtr->SrcPtr;
    StatePtr->SrcPtr = bytePtr + Temp;
    if (Temp < StatePtr->BlockSize) {
        if (Temp < StatePtr->Channels * kMonoHeader)
            return 0;
        /* BUG: a short final block is sized from the full BlockSize, not the bytes present
           (exe 0x00530be6) */
        samplesThisBlock = StatePtr->BlockSize - StatePtr->Channels * 6;
    } else {
        samplesThisBlock = StatePtr->SamplesPerBlock;
    }

    if (StatePtr->Channels == 1) {
        Temp = bytePtr[0];
        if (Temp < kPredictors) {
            MonoState.Coef1 = gaiP4[kCoef1 + Temp];
            MonoState.Coef2 = gaiP4[kCoef2 + Temp];
            Temp = bytePtr[1];
            Temp |= bytePtr[2] << 8;
            MonoState.index = (short)Temp;
            Temp = bytePtr[3];
            Temp |= bytePtr[4] << 8;
            MonoState.sample2 = (short)Temp;
            Temp = bytePtr[5];
            Temp |= bytePtr[6] << 8;
            MonoState.sample1 = (short)Temp;
            OutputPtr = StatePtr->OutputPtr;
            OutputPtr[0] = Temp;
            OutputPtr[1] = MonoState.sample2;
            OutputPtr += 2;
            bytePtr += kMonoHeader;
            if (samplesThisBlock > 2) {
                Count = (samplesThisBlock - 2) >> 1;
                do {
                    Temp = bytePtr[0];
                    ++bytePtr;
                    OutputPtr[0] = ADPCMDecode(&MonoState, Temp >> 4);
                    OutputPtr[1] = ADPCMDecode(&MonoState, Temp);
                    OutputPtr += 2;
                } while (--Count);
            }
            StatePtr->OutputPtr = OutputPtr;
            return samplesThisBlock * 2;
        }
        return 0;
    }

    Temp = bytePtr[0];
    if (Temp < kPredictors) {
        state[0].Coef1 = gaiP4[kCoef1 + Temp];
        state[0].Coef2 = gaiP4[kCoef2 + Temp];
        Temp = bytePtr[1];
        if (Temp < kPredictors) {
            state[1].Coef1 = gaiP4[kCoef1 + Temp];
            state[1].Coef2 = gaiP4[kCoef2 + Temp];
            Temp = bytePtr[2];
            Temp |= bytePtr[3] << 8;
            state[0].index = (short)Temp;
            Temp = bytePtr[4];
            Temp |= bytePtr[5] << 8;
            state[1].index = (short)Temp;
            Temp = bytePtr[6];
            Temp |= bytePtr[7] << 8;
            state[0].sample2 = (short)Temp;
            Temp = bytePtr[8];
            Temp |= bytePtr[9] << 8;
            state[1].sample2 = (short)Temp;
            Temp = bytePtr[10];
            Temp |= bytePtr[11] << 8;
            state[0].sample1 = (short)Temp;
            Temp = bytePtr[12];
            Temp |= bytePtr[13] << 8;
            state[1].sample1 = (short)Temp;
            OutputPtr = StatePtr->OutputPtr;
            OutputPtr[0] = state[0].sample1;
            OutputPtr[1] = state[1].sample1;
            OutputPtr[2] = state[0].sample2;
            OutputPtr[3] = state[1].sample2;
            OutputPtr += 4;
            bytePtr += kMonoHeader * 2;
            if (samplesThisBlock > 2) {
                Count = samplesThisBlock - 2;
                do {
                    Temp = bytePtr[0];
                    ++bytePtr;
                    OutputPtr[0] = ADPCMDecode(&state[0], Temp >> 4);
                    OutputPtr[1] = ADPCMDecode(&state[1], Temp);
                    OutputPtr += 2;
                } while (--Count);
            }
            StatePtr->OutputPtr = OutputPtr;
            return samplesThisBlock * 4;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
