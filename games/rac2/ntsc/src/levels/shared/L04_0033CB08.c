#include "include_asm.h"

extern float func_00283CE0(int);
extern float func_002A8240(void *, void *, float, float, float, int);

float func_L04_0033CB08(void *a0, void *a1, int a2, int a3, float f12, float f13)
{
    return func_002A8240(a0, a1, f12, func_00283CE0(a2) * 0.0009765625f, f13, a3);
}
