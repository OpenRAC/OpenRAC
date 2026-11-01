#include "include_asm.h"
#include "common.h"
typedef int TI128 __attribute__((mode(TI)));
extern float func_002A7878(float, float);
extern float func_00283180(float);
extern float func_00283198(float);

void func_L19_00422178(u8 *a0, float *a1)
{
    float t = func_002A7878(0.0f, (*(float *)(a0 + 0x2C) / *(float *)(*(u8 **)(a0 + 0x24) + 0x24)) * 6.0f);
    float r = func_002A7878(-3.14159274f, 3.14159274f);

    *(TI128 *)a1 = *(TI128 *)(a0 + 0x10);
    a1[0] = a1[0] + t * func_00283180(r);
    a1[1] = a1[1] + t * func_00283198(r);
}
