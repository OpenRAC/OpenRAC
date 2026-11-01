#include "include_asm.h"

/* func_L02_003D6570 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern float D_L02_001B0318 __attribute__((sda));
extern float D_L02_001B031C __attribute__((sda));
extern int D_L02_001B0320 __attribute__((sda));
extern unsigned int D_L02_001B0330 __attribute__((sda));
extern float D_L02_001B0334 __attribute__((sda));
extern float D_L02_001BA500[4];

extern void *func_0029FCC0(char *, int, void *);
extern void func_0029FC20(char *, int, void *);
extern void func_002830C0(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(void *, void *, float);
extern float func_00282DB0(void *, void *);
extern void func_L00_00325AE8(void *, int, float, float);

void func_L02_003D6570(char *arg0, int arg1) {
    float a[4];
    float b[4];
    float c[16];
    float d[4];
    float e[4];
    float f[4];
    float len;
    unsigned int color;

    func_0029FCC0(arg0, arg1, a);
    *(TI128 *)b = *(TI128 *)a;
    func_0029FC20(arg0, arg1, c);
    func_002830C0(d, &D_L02_001B0320, c);
    func_00282CC0(a, a, d);
    func_00282CF0(e, D_L02_001BA500, a);
    func_00282F20(e, e, 1.0f);
    *(TI128 *)f = *(TI128 *)c;
    len = func_00282DB0(e, f);
    if (D_L02_001B0334 < len) {
        color = D_L02_001B0330;
        len = len * len;
        func_L00_00325AE8(a, (color & 0xFFFFFF) | ((int)(len * (float)(int)(color >> 24)) << 24),
                          D_L02_001B031C, D_L02_001B0318);
    }
}
