#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern float func_00283248(float, float);
extern float func_002A7878(float, float);
extern float func_00283180(float);
extern float func_00283198(float);

/* Moves the actor's effect point to a spot around the target position at arg1: the heading from
   the target to the actor plus up to a quarter turn either way, at a distance of 8 plus or minus
   2; the point is copied from arg1 into the state (+0x4A0) and then offset in x and y. */
void func_L02_003F1A88(char *arg0, char *arg1) {
    char *state;
    float angle;
    float radius;

    angle = func_00283248(*(float *)(arg0 + 0x10) - *(float *)arg1,
                          *(float *)(arg0 + 0x14) - *(float *)(arg1 + 4));
    angle += func_002A7878(-0.78539824f, 0.78539824f);
    radius = func_002A7878(-2.0f, 2.0f) + 8.0f;
    state = *(char **)(arg0 + 0x68);
    qcopy(state + 0x4A0, arg1);
    *(float *)(state + 0x4A0) += radius * func_00283180(angle);
    *(float *)(state + 0x4A4) += radius * func_00283198(angle);
}
