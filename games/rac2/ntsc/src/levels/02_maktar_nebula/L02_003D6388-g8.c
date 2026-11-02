#include "include_asm.h"

/* func_L02_003D6388 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern void *func_0029FCC0(char *, int, void *);

extern void func_0029FC20(char *, int, void *);

extern void func_002830C0(void *, void *, void *);

extern void func_00282CC0(void *, void *, void *);

extern void func_00282CF0(void *, void *, void *);

extern void func_00282F20(void *, void *, float);

extern float func_00282DB0(void *, void *);

extern void func_L00_00325AE8(void *, int, float, float);

extern float D_L02_001BA500[4];

extern char D_L02_001B02E0[8] __attribute__((sda));

extern char D_L02_001B0300[8] __attribute__((sda));

extern unsigned int D_L02_001B0310 __attribute__((sda));

extern float D_L02_001B0314 __attribute__((sda));

extern float D_L02_001B02DC __attribute__((sda));

extern float D_L02_001B02D8 __attribute__((sda));

extern float D_L02_001B02F0 __attribute__((sda));

void func_L02_003D6388(char *arg0, int arg1, int arg2) {
    float a[4];
    float b[4];
    float m[16];
    float p[4];
    float q[4];
    float r[4];
    float s[4];
    float d;
    float alpha;
    unsigned int color;

    func_0029FCC0(arg0, arg1, a);
    *(TI128 *)b = *(TI128 *)a;
    func_0029FC20(arg0, arg1, m);
    *(TI128 *)p = *(TI128 *)D_L02_001B02E0;
    *(TI128 *)q = *(TI128 *)D_L02_001B0300;
    if (arg2 != 0) {
        p[1] = -p[1];
        q[1] = -q[1];
    }
    func_002830C0(p, p, m);
    func_00282CC0(a, a, p);
    func_002830C0(q, q, m);
    func_00282CC0(b, b, q);
    func_00282CF0(r, D_L02_001BA500, a);
    func_00282F20(r, r, 1.0f);
    func_00282F20(s, m + 8, -1.0f);
    d = func_00282DB0(r, s);
    if (D_L02_001B0314 < d) {
        color = D_L02_001B0310;
        alpha = (float)(int)(color >> 24);
        color &= 0xFFFFFF;
        d = d * d;
        color |= (int)(d * alpha) << 24;
        func_L00_00325AE8(a, color, D_L02_001B02DC, D_L02_001B02D8);
        func_L00_00325AE8(b, color, D_L02_001B02F0, D_L02_001B02D8);
    }
}
