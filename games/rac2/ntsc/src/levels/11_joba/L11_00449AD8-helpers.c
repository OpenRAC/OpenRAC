#include "common.h"

extern float func_00283248(float f12, float f13);
extern float func_002A7878(float f12, float f13);
extern float func_00283180(float f12);
extern float func_00283198(float f12);

void func_L11_00449AD8(char *object, char *vec)
{
    char *state = *(char **)(object + 0x68);

    if (*(s16 *)(state + 0x36E) != 0) {
        float dx = *(float *)(object + 0x10) - *(float *)(vec + 0x0);
        float dy = *(float *)(object + 0x14) - *(float *)(vec + 0x4);
        float base = func_00283248(dx, dy) + func_002A7878(-0.17453295f, 0.17453295f);
        float scale = 15.0f;
        float t;

        qcopy((state + 0x330), vec);
        *(float *)(state + 0x330) += func_00283180(base) * scale;
        *(float *)(state + 0x334) += func_00283198(base) * scale;
        t = func_002A7878(-4.0f, 4.0f);
        *(float *)(state + 0x338) = *(float *)(object + 0x18) + t;
    } else {
        float dx = *(float *)(object + 0x10) - *(float *)(vec + 0x0);
        float dy = *(float *)(object + 0x14) - *(float *)(vec + 0x4);
        float base = func_00283248(dx, dy) + func_002A7878(-0.78539825f, 0.78539825f);
        float scale = func_002A7878(-2.0f, 2.0f) + 8.0f;

        qcopy((state + 0x330), vec);
        *(float *)(state + 0x330) += scale * func_00283180(base);
        scale = scale * func_00283198(base);
        *(float *)(state + 0x334) += scale;
    }
}
