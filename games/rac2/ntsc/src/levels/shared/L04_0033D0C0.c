#include "common.h"
extern float func_00283180(float);
extern float func_00283198(float);
extern void func_L04_0033CC00(char *,char *,float *,float *,float);
void func_L04_0033D0C0(char *a0, char *a1)
{
    float v[4];
    float w[4];
    float *src = (float *)(a0 + 0x10);

    qcopy(v, src);
    {
        float x = v[0] + func_00283180(*(float *)(a0 + 0xF8)) * 100.0f;
        float angle = *(float *)(a0 + 0xF8);
        v[0] = x;
        v[1] += func_00283198(angle) * 100.0f;
    }
    func_L04_0033CC00(a0, a1, v, w, 1.0f);
}
