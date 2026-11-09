/* burgermath.h -- BurgerLib's FloatAbs, the one definition every engine TU uses. */
#ifndef BURGERMATH_H
#define BURGERMATH_H

#include <math.h>

#ifdef MACBUILD
/* the mac engine calls the out-of-line FloatAbs in physics.c (MathUtil.o) */
extern float FloatAbs(float x);
#else
static inline float FloatAbs(float x)
{
    return fabs((double)x);
}
#ifdef __cplusplus
static inline double FloatAbs(double x)
{
    return fabs(x);
}
#endif
#endif

#endif /* BURGERMATH_H */
