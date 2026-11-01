#include "include_asm.h"

extern float func_00283CE0(int);
extern int func_00283CF0(float);

/* Adjust a non-aligned count through the integer/float conversion helpers. */
int func_L00_003E4B80(int n)
{
    int unit;
    int k;
    int result;
    float f;
    float fu;

    unit = 16;
    result = n;
    if (n % unit != 0) {
        f = func_00283CE0(n);
        fu = (float)unit;
        k = func_00283CF0(f / fu);
        result = (k + 1) * unit;
    }
    return result;
}
