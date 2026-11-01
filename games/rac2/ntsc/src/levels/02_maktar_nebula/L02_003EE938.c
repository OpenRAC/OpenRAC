#include "include_asm.h"

/* func_L02_003EE938 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it. */

typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    unsigned char pad[0x100];
    float f100;
    float f104;
} Pad003EE938;

extern void func_0029F338(char *);
extern void func_L02_003EEAD8(char *);
extern int func_00283CF0(float);
extern void func_002A7750(char *, int, int);
extern int func_L02_003EEFB8(char *);
extern float func_L02_003EEC58(char *);
extern void func_L02_003EEDD0(char *);
extern void func_002AA2E8(float *, float, float);
extern int D_L02_001B30C8 __attribute__((sda));
extern Pad003EE938 D_00138180;

/* The actor's state machine (byte at +0x20): 0 prepares it, 1 waits for the target to come within
   range on its own frame of eight, 2 follows the pad until the state asks to restart. */
void func_L02_003EE938(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    int v;

    switch (*(unsigned char *)(arg0 + 0x20)) {
    case 0:
        if (*(int *)state == -1) {
            func_0029F338(arg0);
            return;
        }
        func_L02_003EEAD8(arg0);
        *(TI128 *)(arg0 + 0x10) = *(TI128 *)(state + 0x20);
        v = func_00283CF0(*(float *)(state + 0x2C) * 1.5f);
        if (v >= 256)
            v = 255;
        *(unsigned char *)(arg0 + 0x30) = v;
        func_002A7750(arg0, 1, -1);
        return;
    case 1:
        if ((D_L02_001B30C8 & 7) == (((int)arg0 >> 8) & 7)) {
            if (func_L02_003EEFB8(arg0)) {
                func_002A7750(arg0, 2, -1);
                *(int *)(state + 0x34) = 0;
            }
        }
        return;
    case 2:
        *(float *)(state + 0x44) = func_L02_003EEC58(arg0);
        func_L02_003EEDD0(arg0);
        if (0.2f < D_00138180.f100 + D_00138180.f104)
            func_002AA2E8((float *)(state + 0x34), 0.0f, 0.33333334f);
        if (*(float *)(state + 4) + 1.0f < *(float *)(state + 0x44))
            func_002A7750(arg0, 1, -1);
        return;
    }
}
