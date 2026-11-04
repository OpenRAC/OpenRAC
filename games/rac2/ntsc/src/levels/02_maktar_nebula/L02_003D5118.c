#include "include_asm.h"

/* func_L02_003D5118 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern void *func_0029FCC0(char *, int, void *);

extern void func_0029FC20(char *, int, void *);

extern void func_00282F20(char *, char *, float);

extern void func_00282CC0(void *, void *, void *);

extern void func_L00_0031BDC0(char *, char *, char *, char *);

extern void func_00282CF0(void *, void *, void *);

extern float func_00282DF0(void *);

extern void func_002A8C28(char *, char *, int, float, float, float, int, int, int);

void func_L02_003D5118(char *arg0, int arg1) {
    char *state = *(char **)(arg0 + 0x68);
    float a[4];
    float d[4];
    float m[16];
    float p[4];
    float q[4];
    float r[4];
    float s[4];
    float t;

    func_0029FCC0(arg0, arg1, a);
    func_0029FC20(arg0, arg1, m);
    *(TI128 *)p = *(TI128 *)m;
    func_00282F20((char *)p, (char *)p, 5.3f);
    func_00282CC0(d, a, p);
    func_L00_0031BDC0((char *)q, (char *)a, (char *)d, *(char **)(state + 0x30C) + 0x10);
    func_00282CF0(r, a, q);
    func_00282CF0(s, a, d);
    t = func_00282DF0(r);
    t = t / func_00282DF0(s);
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    func_002A8C28(arg0, (char *)q, 0x10001, t * 0.2f + 0.3f,
                  (float)*(unsigned char *)(state + 0x34), 1.0f, 0, 1, 0);
}
