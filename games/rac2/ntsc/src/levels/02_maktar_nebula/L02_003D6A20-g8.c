#include "include_asm.h"

/* func_L02_003D6A20 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it;
   -g gives the two nops retail has before its `div.s` (a -g unit's line labels). */

typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    TI128 p0;
    TI128 t0;
    TI128 p1;
    TI128 t1;
    int e0;
    int e1;
    int e2;
    int e3;
    int z50;
    int z54;
    int z58;
    float f5c;
    float f60;
    int z64;
    float f68;
    float f6c;
    char pad70[0x20];
} L02_003D6A20_Seg;

extern int D_L02_001B0344 __attribute__((sda));

extern int D_L02_001B0370 __attribute__((sda));

extern int D_L02_001B0374 __attribute__((sda));

extern TI128 D_L02_002412A0[];

extern TI128 D_L02_002415C0[];

extern void func_00281DE8(void *, int, int, int);

extern float func_00283CE0(int);

extern int func_00283D38(int, int, float);

extern void func_00280B90(void *, int, int);

void func_L02_003D6A20(void) {
    L02_003D6A20_Seg s;
    int i;
    int prev;
    float step;
    float t;
    int r;
    TI128 *t1;
    TI128 *t2;

    if (D_L02_001B0344 >= 2) {
        func_00281DE8(&s, 0x2B, 1, 0x80);
        s.z50 = 0;
        s.z54 = 0;
        s.z58 = 0;
        s.f5c = 1.0f;
        s.f60 = 1.0f;
        s.z64 = 0;
        s.f68 = 1.0f;
        s.f6c = 1.0f;
        prev = 0;
        step = 1.0f / func_00283CE0(D_L02_001B0344 - 1);
        for (i = 0; i < D_L02_001B0344 - 1; i++) {
            t1 = D_L02_002415C0;
            t2 = D_L02_002412A0;
            s.p0 = t1[i];
            s.t0 = t2[i];
            s.p1 = t1[i + 1];
            s.t1 = t2[i + 1];
            t = func_00283CE0(i) * step;
            r = func_00283D38(D_L02_001B0374, D_L02_001B0370, t * t);
            s.e1 = prev;
            s.e0 = prev;
            s.e3 = r;
            s.e2 = r;
            prev = r;
            func_00280B90(&s, 0, 0);
        }
    }
}
