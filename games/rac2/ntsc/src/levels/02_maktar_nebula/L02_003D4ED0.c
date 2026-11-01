#include "include_asm.h"

/* func_L02_003D4ED0 is a unit of its own: it needs the opt-in quadword-copy profile
   (-frac2-qcopy, #262), which gives retail's grouped `lq`/`sq` with register addresses. */

typedef int TI128 __attribute__((mode(TI)));

extern float func_00282E48(char *, char *);

extern void func_00282CF0(void *, void *, void *);

extern void func_00282F20(char *, char *, float);

extern void func_00282CC0(void *, void *, void *);

extern float func_00283180(float);

extern float func_00283198(float);

void func_L02_003D4ED0(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    char *dst = state + 0x2D0;
    char *src = state + 0x2C0;
    float v[4];

    if (func_00282E48(dst, src) < 0.8333334f) {
        *(TI128 *)dst = *(TI128 *)src;
    } else {
        func_00282CF0(v, src, dst);
        func_00282F20((char *)v, (char *)v, 0.8333334f);
        v[0] = v[0] * func_00283180(*(float *)(state + 0x308)) -
               v[1] * func_00283198(*(float *)(state + 0x308));
        v[1] = v[0] * func_00283198(*(float *)(state + 0x308)) +
               v[1] * func_00283180(*(float *)(state + 0x308));
        func_00282CC0(dst, dst, v);
    }
}
