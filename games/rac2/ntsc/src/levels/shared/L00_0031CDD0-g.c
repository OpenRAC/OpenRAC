#include "include_asm.h"

extern void func_00282CF0(float *, const float *, const float *);
extern float func_00282E20(const float *);

float func_L00_0031CDD0(const float *a, const float *b, float *out, float d, float e) {
    float buf[4];
    float t;
    float len;

    func_00282CF0(buf, b, a);
    len = func_00282E20(buf);
    t = len / d;
    if (out != 0)
        *out = t;
    if (t == 0.0f)
        return 0.0f;
    return -((a[2] - b[2]) + e * (t * t) * 0.5f) / t;
}
