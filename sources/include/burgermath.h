/* burgermath.h -- BurgerLib's FloatAbs, the one definition every engine TU uses. */
#ifndef BURGERMATH_H
#define BURGERMATH_H

#include <math.h>

static inline float FloatAbs(float x)
{
    return fabs((double)x);
}

#endif /* BURGERMATH_H */
