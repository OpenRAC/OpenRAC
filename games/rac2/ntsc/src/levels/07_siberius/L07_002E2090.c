/* func_L07_002E2090 (112 bytes): spawns a record through func_L04_0030C4C0 (here
   ref_L07_002E2090_00) with two zeroed vectors whose z is +-0.4 of the z distance
   between the two points, and returns what the spawner returns. */
#include "common.h"

extern void *ref_L07_002E2090_00(void *, void *, void *, void *, void *, void *, int, void *, unsigned int);

void *func_L07_002E2090(void *a0, float *a1, float *a2, void *a3, int t0, void *t1, unsigned int t2) {
    float v1[4];
    float v2[4];
    float f;
    qzero_nc(v1);
    qzero_nc(v2);
    f = (a2[2] - a1[2]) * 0.4f;
    v1[2] = f;
    v2[2] = -f;
    return ref_L07_002E2090_00(a0, a1, v1, a2, v2, a3, t0, t1, t2);
}
