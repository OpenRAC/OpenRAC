#include "include_asm.h"

/* func_L02_003D47B0 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

extern float func_002A7878(float, float);

extern void func_002A7750(char *, int, int);

extern void func_002A0488(char *, int, int, int);

extern void func_L02_003D4AF0(char *, int);

extern void func_L02_003D5570(char *);

extern float func_00283248(float, float);

typedef struct {
    char pad0[0x80];
    float x;
    float y;
} L02_003D7900_Hero;

extern L02_003D7900_Hero D_00189E20;

extern int D_L02_001B01C0 __attribute__((sda));

void func_L02_003D47B0(char *arg0, int arg1) {
    char *state = *(char **)(arg0 + 0x68);
    float r;
    L02_003D7900_Hero *hero;

    *(float *)(state + 0x20) = (float)*(short *)(state + 0x24);
    r = func_002A7878(0.0f, 1.5707965f);
    *(float *)(state + 0x304) = 0.0f;
    *(short *)(state + 0x32C) = 0;
    *(int *)(state + 0x300) = 0;
    *(short *)(state + 0x32A) = 0;
    *(short *)(state + 0x98) = 0;
    *(float *)(state + 0x2F4) = r;
    func_002A7750(arg0, 2, 1);
    func_L02_003D4AF0(arg0, 1);
    *(unsigned char *)(state + 0x7A) = 0;
    *(unsigned char *)(state + 0x2E) = 1;
    *(unsigned char *)(arg0 + 0x31) = 1;
    *(unsigned short *)(arg0 + 0x34) = *(unsigned short *)(arg0 + 0x34) & 0xFFFE;
    *(int *)(arg0 + 0x98) = *(int *)(*(char **)(arg0 + 0x24) + 0x10);
    *(unsigned short *)(arg0 + 0x34) = (*(unsigned short *)(arg0 + 0x34) | 0x1000) & 0xFFFD;
    *(unsigned char *)(arg0 + 0x7F) = 0;
    /* The destination is named so that its address is formed first, as in retail: written
       inline, the source's small-data address goes into a register while the right-hand side
       is expanded, before the destination's, and the two address registers swap. */
    {
        TI128 *dst = (TI128 *)(arg0 + 0x10);

        *dst = *(TI128 *)&D_L02_001B01C0;
    }
    hero = &D_00189E20;
    *(float *)(arg0 + 0xF8) = func_00283248(hero->x - *(float *)(arg0 + 0x10),
                                            hero->y - *(float *)(arg0 + 0x14));
    *(int *)(state + 0x310) = 0;
    *(short *)(state + 0x318) = 0;
    *(short *)(state + 0x31A) = 0;
    func_L02_003D5570(arg0);
    if (*(short *)(state + 0x60) != 0) {
        *(short *)(state + 0x60) = 0;
        func_002A0488(arg0, *(unsigned char *)(state + 0x64),
                      *(unsigned char *)(state + 0x65),
                      *(unsigned char *)(state + 0x66));
        *(short *)(state + 0x62) = 1;
    }
    *(short *)(state + 0x330) = arg1;
}
