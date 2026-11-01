#include "include_asm.h"

/* func_L23_00418BC8 is a unit of its own: it reads $gp data and needs the opt-in quadword-copy
   profile and its scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt).
   The stack vectors stay plain addresses: retail's second names for them are gcse's reaching
   registers, so a pointer local here only moves them away from retail's. */

typedef int TI128 __attribute__((mode(TI)));

extern float D_L23_001B16B8 __attribute__((sda));

extern float func_00282E48(float *, float *);
extern void func_00282CC0(float *, float *, float *);
extern void func_00282CF0(float *, float *, float *);
extern void func_00282F20(float *, float *, float);
extern void func_L23_00418650(char *, float);
extern int func_002761F8(char *, char *, int, int, int);

int func_L23_00418BC8(char *obj, TI128 *pa, TI128 *pb) {
    char *state = *(char **)(obj + 0x68);
    TI128 loc[6];
    int found;

    if (func_00282E48((float *)pa, (float *)pb) > 70.0f)
        return 0;
    loc[0] = *pb;
    func_L23_00418650((char *)&loc[0], *(float *)(state + 0x54) + *(float *)(state + 0x50));
    func_00282CF0((float *)&loc[1], (float *)&loc[0], (float *)pa);
    func_00282F20((float *)&loc[1], (float *)&loc[1], D_L23_001B16B8);
    loc[4] = *pa;
    loc[2] = *pa;
    func_L23_00418650((char *)&loc[2], *(float *)(state + 0x54) + *(float *)(state + 0x50));
    found = 0;
    do {
        float d = func_00282E48((float *)&loc[4], (float *)&loc[0]);

        if (d < D_L23_001B16B8) {
            if (d < 0.0f)
                return 1;
            func_00282F20((float *)&loc[5], (float *)&loc[1], d);
            found = 1;
            func_00282CC0((float *)&loc[3], (float *)&loc[4], (float *)&loc[5]);
            func_L23_00418650((char *)&loc[3], *(float *)(state + 0x54) + *(float *)(state + 0x50));
        } else {
            func_00282CC0((float *)&loc[3], (float *)&loc[4], (float *)&loc[1]);
            func_L23_00418650((char *)&loc[3], *(float *)(state + 0x54) + *(float *)(state + 0x50));
        }
        if (func_002761F8((char *)&loc[2], (char *)&loc[3], 0x12, 0, 0) != 0)
            return 0;
        loc[2] = loc[3];
        func_00282CC0((float *)&loc[4], (float *)&loc[4], (float *)&loc[1]);
    } while (found == 0);
    return 1;
}
