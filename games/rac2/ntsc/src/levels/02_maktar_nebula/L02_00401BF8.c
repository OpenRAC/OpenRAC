#include "include_asm.h"

/* func_L02_00401BF8 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));
typedef unsigned char u8;
typedef int s32;

extern char D_00188660[];
extern int func_002E5CF8(int, int, char *);
extern float func_00283B98(float, float);
extern float func_00283198(float);

void func_L02_00401BF8(char *a0) {
    char *s0 = *(char **)(a0 + 0x68);
    char *e = D_00188660 + *(s32 *)(s0 + 0x16C) * 0x70;
    float f;

    if (*(char **)(e + 0x88) != a0 || *(u8 *)(e + 0x74) == 0)
        *(s32 *)(s0 + 0x16C) = func_002E5CF8(1, 4, a0);
    *(TI128 *)(a0 + 0x10) = *(TI128 *)(s0 + 0x120);
    *(TI128 *)(a0 + 0xF0) = *(TI128 *)(s0 + 0x130);
    f = func_00283B98(*(float *)(s0 + 0x148), 0.008726648f);
    *(float *)(s0 + 0x148) = f;
    *(float *)(a0 + 0xF0) += func_00283198(f) * 0.05f;
    f = func_00283B98(*(float *)(s0 + 0x144), 0.0145444125f);
    *(float *)(s0 + 0x144) = f;
    *(float *)(a0 + 0xF4) += func_00283198(f) * 0.05f;
    f = func_00283B98(*(float *)(s0 + 0x140), 0.020362178f);
    *(float *)(s0 + 0x140) = f;
    *(float *)(a0 + 0x18) += func_00283198(f) * 0.25f;
}
