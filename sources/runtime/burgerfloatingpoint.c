// COMPILE: -opt level=4,intrinsics,noieeefp -inline auto -align 4 -Cpp_exceptions off -proc pentium4 -lang c++ -bool off -str pool
// COMPILE-WHY: -str pool, the band's string literals are one packed pool per TU (exe .rdata, Burger archive members);runtime P band, the one line for BurgerLib (user ruling 2026-09-27; Research/runtime-compiler-ceiling.md)
// LINK: after atutf162; data EVNOVA 0x0057d388
/* burgerfloatingpoint.c -- BurgerLib FpFloatingPoint: the arc tangent table lookup, vector and matrix helpers; not linked. Notes: Research/notes/burgerfloatingpoint.md */

#include <math.h>
#include "burgertypes.h"
#include "burger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Vector2D_t {
    float x, y;
} Vector2D_t;

typedef struct Vector3D_t {
    float x, y, z;
} Vector3D_t;

typedef struct Matrix3D_t {
    Vector3D_t x, y, z;
} Matrix3D_t;

typedef struct Quat_t {
    float x, y, z, w;
} Quat_t;

typedef struct FixedVector3D_t {
    long x, y, z;
} FixedVector3D_t;

typedef struct Euler_t {
    int x, y, z;
} Euler_t;

#define kTanTableSize    2048
#define kTanFullCircle   2048.0f
#define kTanThreeQuarter 1536.0f
#define kTanHalfCircle   1024.0f
#define kTanQuarter      512.0f
#define kTanHalfTurn     0x400
#define kGimbalLock      0x200
#define kGimbalLow       0x600
#define kAngleMask       0x7ff

static const double kSqrtHalf = 0.70710677f;

extern float FAngleArray[kTanTableSize + 1];
extern int FloatToInt(float Input);
extern float SqrtFast(float Input);
extern long FloatArcSine(float s);
extern long FloatArcCosine(float c);

float FloatATan2(float x, float y)
{
    if (x < 0.0f) {
        x = -x;
        if (y >= 0.0f) {
            if (y <= x)
                return kTanHalfCircle - FAngleArray[FloatToInt(y / x * kTanFullCircle)];
            return FAngleArray[FloatToInt(x / y * kTanFullCircle)] + kTanQuarter;
        }
        y = -y;
        if (y <= x)
            return FAngleArray[FloatToInt(y / x * kTanFullCircle)] + kTanHalfCircle;
        return kTanThreeQuarter - FAngleArray[FloatToInt(x / y * kTanFullCircle)];
    }
    if (y >= 0.0f) {
        if (y <= x) {
            if (y == 0.0f)
                return 0.0f;
            return FAngleArray[FloatToInt(y / x * kTanFullCircle)];
        }
        return kTanQuarter - FAngleArray[FloatToInt(x / y * kTanFullCircle)];
    }
    y = -y;
    if (y <= x)
        return kTanFullCircle - FAngleArray[FloatToInt(y / x * kTanFullCircle)];
    return FAngleArray[FloatToInt(x / y * kTanFullCircle)] + kTanThreeQuarter;
}

float Vector2DGetRadius(const Vector2D_t *Input)
{
    return sqrt(Input->x * Input->x + Input->y * Input->y);
}

void Vector3DFromFixedVector3D(Vector3D_t *Output, const FixedVector3D_t *Input)
{
    Output->x = Input->x * (1.0f / 65536.0f);
    Output->y = Input->y * (1.0f / 65536.0f);
    Output->z = Input->z * (1.0f / 65536.0f);
}

void Matrix3DSetFromQuat(Matrix3D_t *Output, const Quat_t *Input)
{
    float x, y, z, w;
    float x2, y2, z2;
    float xx, xy, xz, yy, yz, zz, wx, wy, wz;

    x = Input->x;
    y = Input->y;
    z = Input->z;
    w = Input->w;
    x2 = x * 2.0f;
    y2 = y * 2.0f;
    z2 = z * 2.0f;
    xx = x * x2;
    xy = x * y2;
    xz = x * z2;
    yy = y * y2;
    yz = y * z2;
    zz = z * z2;
    wx = w * x2;
    wy = w * y2;
    wz = w * z2;
    Output->x.x = 1.0f - yy - zz;
    Output->x.y = xy - wz;
    Output->x.z = xz + wy;
    Output->y.x = xy + wz;
    Output->y.y = 1.0f - xx - zz;
    Output->y.z = yz - wx;
    Output->z.x = xz - wy;
    Output->z.y = yz + wx;
    Output->z.z = 1.0f - xx - yy;
}

void EulerFromMatrix3D(Euler_t *Output, const Matrix3D_t *Input)
{
    float sinX, sq, cosX, recip, a, b;
    int angle;

    sinX = Input->y.z;
    sq = sinX * sinX;
    if (sq >= 1.0f) {
        if (sinX > 0.0f)
            Output->x = kGimbalLock;
        else
            Output->x = kGimbalLow;
        Output->y = 0;
        Output->z = ((int)FloatATan2(-Input->x.y, Input->x.x) - kGimbalLock) & kAngleMask;
        return;
    }
    cosX = SqrtFast(1.0f - sq);
    if (cosX == 0.0f) {
        recip = 1.0f;
    } else if (cosX < 1.0f) {
        recip = 1.0f / cosX;
    } else {
        cosX = 1.0f;
        recip = 1.0f;
    }
    if (cosX > kSqrtHalf) {
        angle = FloatArcSine(sinX) >> kFixedShift;
        if (cosX < 0.0f)
            angle = kTanHalfTurn - angle;
    } else {
        angle = FloatArcCosine(cosX) >> kFixedShift;
        if (sinX <= 0.0f)
            angle = -angle;
    }
    Output->x = angle & kAngleMask;

    a = Input->z.z * recip;
    b = Input->x.z * recip;
    if (a > kSqrtHalf || a < -kSqrtHalf) {
        angle = FloatArcSine(b) >> kFixedShift;
        if (a < 0.0f)
            angle = kTanHalfTurn - angle;
    } else {
        angle = FloatArcCosine(a) >> kFixedShift;
        if (b < 0.0f)
            angle = -angle;
    }
    Output->y = angle & kAngleMask;

    a = Input->y.y * recip;
    b = Input->y.x * recip;
    if (a > kSqrtHalf || a < -kSqrtHalf) {
        angle = FloatArcCosine(a) >> kFixedShift;
        if (b < 0.0f)
            angle = -angle;
    } else {
        angle = FloatArcSine(b) >> kFixedShift;
        if (a < 0.0f)
            angle = kTanHalfTurn - angle;
    }
    Output->z = -angle & kAngleMask;
}

#ifdef __cplusplus
}
#endif
