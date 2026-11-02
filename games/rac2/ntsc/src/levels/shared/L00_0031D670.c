#include "common.h"

typedef unsigned int u128 __attribute__((mode(TI)));

extern float func_002AA140(float, float, float);

void func_L00_0031D670(f32 *out, const f32 *a, const f32 *b, f32 factor) {
    u128 left, right;
    f32 *lf = (f32 *)&left;
    f32 *rf = (f32 *)&right;
    left = *(const u128 *)a;
    right = *(const u128 *)b;

    out[0] = func_002AA140(lf[0], rf[0], factor);
    out[1] = func_002AA140(lf[1], rf[1], factor);
    out[2] = func_002AA140(lf[2], rf[2], factor);
    out[3] = func_002AA140(lf[3], rf[3], factor);
}
