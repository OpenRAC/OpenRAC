#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern float func_00283248(float f12, float f13);
extern float func_002A7878(float f12, float f13);
extern float func_00283180(float f12);
extern float func_00283198(float f12);

void func_L11_00452F30(char *a0, TI128 *a1)
{
    char *s0 = *(char **)(a0 + 0x68);
    float f12, f13;
    if (*(s16 *)(s0 + 0x2D8) != 0) {
        float sum, scale;
        f12 = *(float *)(a0 + 0x10) - *(float *)a1;
        f13 = *(float *)(a0 + 0x14) - *(float *)((char *)a1 + 4);
        sum = func_00283248(f12, f13) + func_002A7878(-0.78539825f, 0.78539825f);
        scale = 15.0f;
        qcopy((s0 + 0x2A0), a1);
        *(float *)(s0 + 0x2A0) += func_00283180(sum) * scale;
        *(float *)(s0 + 0x2A4) += func_00283198(sum) * scale;
        *(float *)(s0 + 0x2A8) = func_002A7878(0.0f, 4.0f) + 466.0f;
    } else {
        float sum, k;
        f12 = *(float *)(a0 + 0x10) - *(float *)a1;
        f13 = *(float *)(a0 + 0x14) - *(float *)((char *)a1 + 4);
        sum = func_00283248(f12, f13);
        sum += func_002A7878(-0.78539825f, 0.78539825f);
        k = func_002A7878(-2.0f, 2.0f);
        k += 8.0f;
        qcopy((s0 + 0x2A0), a1);
        *(float *)(s0 + 0x2A0) = *(float *)(s0 + 0x2A0) + k * func_00283180(sum);
        k *= func_00283198(sum);
        *(float *)(s0 + 0x2A4) += k;
    }
}
