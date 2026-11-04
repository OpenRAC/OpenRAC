#include "include_asm.h"

#include "common.h"

/* Small-data unit: the bodies whose state lives at $gp-relative offsets. */

extern int D_L15_001B1530 __attribute__((sda));
extern int D_L15_001B1534 __attribute__((sda));
extern int D_L15_001B15C4 __attribute__((sda));

extern unsigned char D_L15_00221E10[];
extern unsigned char D_L15_0029FD80[];

extern int func_80381310(int a0);
extern void func_80380770(int a0, void *a1);
extern void func_80383FB8(void);
extern void func_803807B0(void);

/* Publishes the level's frame budget: seeds the counters and points the stage at
   the record the factory returned, then runs the three stage hooks. */
/* func_L15_0042FD40 is defined in L15_0042FD40-g8.c. */

extern int D_L15_001B1520 __attribute__((sda));
extern int D_L15_001B1524;
extern int D_L15_001B1528;
extern int D_L15_001B152C;
extern u8 D_L15_001395B8[];

/* Copies the four clamped channel bytes into the render block's colour slots. */

extern char *func_002A9640(char *a0, int a1, int a2);
extern void func_002AB8C0(char *a0, char *a1);
extern void func_002AB800(char *a0, char *a1);
extern int func_002E5DA0(int index, int value, char *object, int kind);
/* The effect call is declared as returning int on purpose: its (unused) result
   stays in $v0, which is what keeps the $gp load below out of $v0. */
extern int func_L05_00424990(void *a0, void *a1, int a2, int a3, int a4, int a5, int a6, int a7,
                             int a8, int a9, int a10, float f12);
extern void func_L05_00421908(char *a0, int a1, int a2);
extern int func_L05_0037BBB8(char *a0);
extern void func_L05_0037BE98(char *a0);
extern void func_L05_0037AE40(char *a0);

/* The moby's death timer: it ticks the stored time down and, once it reaches zero,
   fires the object's effect and takes the moby down, which it reports. */
int func_L15_0042FFB8(char *obj, int flag)
{
    char *s0;
    float *s2;
    char *s3;
    char *v;
    short v1;
    int ret = 0;

    s3 = func_002A9640(obj, 0x230000, 0);
    *(unsigned char *)(obj + 0xA8) = 0xFF;
    v = *(char **)(obj + 0x68);
    s0 = *(char **)(v + 0x10);
    s2 = *(float **)(v + 0x0);
    if (s0 != 0) {
        func_002AB8C0(obj, s0);
    }
    if (s3 != 0) {
        if (s0 != 0) {
            *(unsigned char *)(s0 + 7) = 0xFF;
            func_002AB800(obj, s0);
        }
        *s2 = *s2 - *(float *)(s3 + 0x2C);
        v1 = *(short *)(obj + 0xAA);
        if (v1 != 0xB73) {
            if (v1 == 0x1211) {
                func_002E5DA0(5, 0, obj, 0x1211);
            } else {
                func_002E5DA0(0xD, 0, obj, 0xFBF);
            }
        }
        if (*s2 <= 0.0f) {
            *s2 = 0.0f;
            func_L05_00424990(obj + 0x10, 0, 0x32, 0x1E, 0x1E, 0x1E, 0x28, 1, 1, 0, 0, 0.75f);
            if (D_L15_001B1520 == 0) {
                func_L05_00421908(obj, 1, 0xF);
            } else {
                func_L05_00421908(obj, 0, 0xF);
            }
            if (func_L05_0037BBB8(obj)) {
                func_L05_0037BE98(obj);
            }
            if (flag == 0) {
                func_L05_0037AE40(obj);
            }
            ret = 1;
        }
    }
    return ret;
}
