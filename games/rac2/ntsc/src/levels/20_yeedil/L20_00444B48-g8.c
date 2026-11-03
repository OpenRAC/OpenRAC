#include "common.h"

extern float D_L20_001B20A0 __attribute__((sda));
extern float func_00283CE0(int);

/* The EE ABI gives the integer and float arguments separate register banks. */
float func_L20_00444B48(int start, float startValue, float endValue, int end)
{
    if (D_L20_001B20A0 <= (float)start)
        return startValue;
    if ((float)end <= D_L20_001B20A0)
        return endValue;
    {
        float first = func_00283CE0(start);
        float span = func_00283CE0(end) - func_00283CE0(start);
        float delta = endValue - startValue;
        /* Keep this source line together: its debug label precedes the subtraction. */
        float numerator = D_L20_001B20A0 - first; float fraction = numerator / span;
        return startValue + delta * fraction;
    }
}
