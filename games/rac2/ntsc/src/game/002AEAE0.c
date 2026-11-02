#include "include_asm.h"

/* Round x to a decimal place count: scale = 10^a0, then round(x * scale) / scale.
   -g for the nop pair before div.s. */
extern int func_00283CF0(float x);

float func_002AEAE0(int a0, float x)
{
    int r = 1;
    float scale;
    int n;
    if (a0 > 0) {
        do {
            r *= 10;
        } while (--a0 != 0);
    }
    scale = (float)r;
    x += 1.0f / (scale + scale);
    n = func_00283CF0(x * scale);
    x = (float)n;
    return x / scale;
}
