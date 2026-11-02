#include "include_asm.h"

/* func_L02_003E8CE8 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give retail's grouped `lq`/`sq`
   with register addresses and keep the instructions around the copy on their own side of it;
   -g gives the two nops retail has before its `div.s` (a -g unit's line labels). */

typedef int TI128 __attribute__((mode(TI)));

typedef struct {
    unsigned char pad000[0xE0];
    float vecE0[4];
    unsigned char pad0F0[0x5C0 - 0xF0];
    char *target5C0;
} Player003E8CE8;

extern char *D_L02_001B359C[3] __attribute__((sda));
extern float D_L02_001B0C00 __attribute__((sda));
extern float D_L02_001B0C04 __attribute__((sda));
extern float D_L02_001B0C08 __attribute__((sda));
extern float D_L02_001B0C0C __attribute__((sda));
extern Player003E8CE8 D_00189E20;

extern void func_00282CF0(void *, void *, void *);
extern float func_00282DF0(void *);
extern void func_00282F20(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_002AA2E8(float *, float, float);

/* Steers the entity named by the state's first word toward the actor's position (lifted by
   D_L02_001B0C00): when it is the entity the player holds, the offset is first eased through
   func_002AA2E8; the entity's position then gets a spring term (distance times 1/3600 times
   D_L02_001B0C08) and a damping term (the speed over D_L02_001B0C0C) added to the velocity at
   state+0x10, and the velocity added to the position. */
void func_L02_003E8CE8(char *arg0) {
    char *state = *(char **)(arg0 + 0x68);
    char *target = D_L02_001B359C[0] + (*(int *)state << 8);
    float pos[4];
    float vec[4];
    float diff[4];
    float spring[4];
    float damp[4];
    float step;
    float *q;
    float a;
    float b;

    *(TI128 *)pos = *(TI128 *)(arg0 + 0x10);
    pos[2] += D_L02_001B0C00;
    if (D_00189E20.target5C0 == target) {
        func_00282CF0(vec, D_00189E20.vecE0, pos);
        step = func_00282DF0(vec);
        func_002AA2E8(&step, 0.1f, D_L02_001B0C04);
        func_00282F20(vec, vec, step);
        func_00282CC0(vec, vec, pos);
    } else {
        *(TI128 *)vec = *(TI128 *)pos;
    }
    q = vec;
    func_00282CF0(diff, target + 0x10, q);
    a = func_00282DF0(diff);
    b = func_00282DF0(state + 0x10);
    a = a * 0.00027777778f;
    b = b / D_L02_001B0C0C;
    func_00282F20(spring, diff, -(D_L02_001B0C08 * a));
    func_00282F20(damp, state + 0x10, -b);
    func_00282CC0(state + 0x10, state + 0x10, spring);
    func_00282CC0(state + 0x10, state + 0x10, damp);
    func_00282CC0(target + 0x10, target + 0x10, state + 0x10);
}
