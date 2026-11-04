/* Choose a randomized point around the supplied full vector and store it in the private state.
 * The shared helper transfers the complete aligned 16-byte input vector.
 */
#include "common.h"

extern float func_00283248(float,float),func_002A7878(float,float);
extern float func_00283180(float),func_00283198(float);

void func_L02_0040BAB8(u8 *a0, float *a1) {
    float v, t, h, k;
    u8 *q;

    v = func_00283248(*(float *)(a0 + 0x10) - a1[0], *(float *)(a0 + 0x14) - a1[1]);
    v = v + func_002A7878(-0.7853982448577881f, 0.7853982448577881f);
    t = func_002A7878(-2.0f, 2.0f) + 8.0f;
    q = *(u8 **)(a0 + 0x68);
    qcopy(q + 0x2C0, a1);
    h = t * func_00283180(v);
    *(float *)(q + 0x2C0) = *(float *)(q + 0x2C0) + h;
    k = func_00283198(v);
    *(float *)(q + 0x2C4) = *(float *)(q + 0x2C4) + t * k;
}
