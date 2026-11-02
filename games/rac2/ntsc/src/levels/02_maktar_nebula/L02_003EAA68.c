#include "include_asm.h"

/* func_L02_003EAA68 is a unit of its own: it needs the opt-in quadword-copy profile with its
   scheduling barrier and the clear's barrier (-frac2-qcopy -frac2-qcopy-barrier
   -frac2-zero-ti-barrier, config/cflags.txt). The zero clear at the top is retail's
   `sq $zero,0($sp)` followed at once by a reload of the group byte, and the entity's +0x10
   quadword reaches the +0x30 of its private data through register addresses; with the copy a
   barrier, retail's own statement order (copy, +0x3C, +0x28) keeps `state + 0x24` after the copy.
   D_L02_001B359C and D_L02_001B35B8 are `sda` arrays of unknown size read as [0]: bare-symbol
   loads, and the `$gp` form in the delay slot of the first walk's `beq`. */

typedef int TI128 __attribute__((mode(TI)));

extern unsigned short *D_L02_001FC060[];
extern char *D_L02_001B359C[] __attribute__((sda));
extern int D_L02_001B35B8[] __attribute__((sda));

extern void func_00282CC0(void *, void *, void *);
extern void func_00282D30(void *, void *, float);
extern void func_00282F20(void *, void *, float);
extern float func_00283B98(float, float);

void func_L02_003EAA68(unsigned char *arg0) {
    char *state;
    float sum[4];
    float off[4];
    int first;
    unsigned short *p;
    char *e;

    if (arg0[0x21] == 0xFF)
        return;
    first = 0;
    state = *(char **)(arg0 + 0x68);
    *(TI128 *)sum = 0;
    for (p = D_L02_001FC060[arg0[0x21]];
         arg0[0x21] != 0xFF && arg0[0x21] <= D_L02_001B35B8[0] && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L02_001FC060[arg0[0x21]]);
         p++) {
        e = D_L02_001B359C[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (*(short *)(e + 0xAA) == 0xAB0) {
            char *es = *(char **)(e + 0x68);

            *(TI128 *)(es + 0x30) = *(TI128 *)(e + 0x10);
            *(float *)(es + 0x3C) = *(float *)(arg0 + 0xF8);
            *(char **)(es + 0x28) = state + 0x24;
            if (*(int *)es != 0) {
                if (first == 0) {
                    *(int *)es = 2;
                    first = 1;
                }
                func_00282CC0(sum, sum, e + 0x10);
                e[0x20] = 2;
            } else {
                e[0x20] = 1;
            }
        }
    }
    func_00282D30(sum, sum, 0.5f);
    func_00282F20(off, arg0 + 0xD0, 0.5f);
    off[2] -= 0.5f;
    for (p = D_L02_001FC060[arg0[0x21]];
         arg0[0x21] != 0xFF && arg0[0x21] <= D_L02_001B35B8[0] && p != 0 &&
         ((p != 0 && (short)p[-1] >= 0) || p == D_L02_001FC060[arg0[0x21]]);
         p++) {
        e = D_L02_001B359C[0] + ((*p & 0x7FFF) << 8);
        if (e == 0)
            break;
        if (*(short *)(e + 0xAA) == 0xAB0 && **(int **)(e + 0x68) == 0) {
            ((unsigned char *)e)[0x30] = 0xFF;
            *(float *)(e + 0xF8) = func_00283B98(*(float *)(e + 0xF8), 1.5707965f);
            *(float *)(e + 0xF0) = 3.1415927f;
            func_00282CC0(e + 0x10, sum, off);
            *(int *)(e + 0x98) = 0;
            *(unsigned short *)(e + 0x34) = (*(unsigned short *)(e + 0x34) | 0x41) & 0xEFFF;
        }
    }
}
