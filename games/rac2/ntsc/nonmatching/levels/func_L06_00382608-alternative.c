/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L06_00382608: 616 of 584 bytes (32 bytes over), 0 of 2 places. Control flow, calls, constants and
 * the three stack vectors (prev sp+0, cur sp+0x10, vec sp+0x20) match; retail's callee-saved
 * allocation does not: retail hoists nothing out of the route loop except (short)var << 16 (spilled
 * to 0x38) and reloads the flags word from its 0x30 slot every iteration, with a1/t0 kept in
 * s5; mine hoists a3 & 7, the 1.2f constant and the local addresses into registers/spill slots.
 * Tried: s16 local vs s16 callee parameter (the parameter form is right), declaration order of
 * the vectors (retail order is prev, cur, vec). Needs a lever that stops the loop-invariant motion.
 */
#include "common.h"
extern void func_002E3EF0(void *);
extern void func_0032D8A8(void *, void *, float);
extern void func_0032D5C8(void *, void *, void *);
extern void func_00378758(void *, void *, int, void *, float, int, s16, int);
extern u8 D_L06_001AA660[];
extern u8 D_L06_001B9A90[];
extern u8 *D_L06_001FE500[];

void func_L06_00382608(u8 *a0, int a1, void *a2, int a3, int a4, float a5) {
    u8 *route;
    s32 var;
    s32 i;
    s32 next;
    s32 n;
    float prev[4] __attribute__((aligned(16)));
    float cur[4] __attribute__((aligned(16)));
    float vec[4] __attribute__((aligned(16)));

    if (a1 == -1) {
        func_002E3EF0(D_L06_001AA660);
        return;
    }
    route = D_L06_001FE500[a1];
    var = (a3 & 0x40) ? -1 : 2;
    qcopy(prev, route + 0x10);
    for (i = 0; i < *(s32 *)route; i = next) {
        func_0032D8A8(vec, D_L06_001B9A90, a5 * 1.2f);
        qcopy(cur, route + (i << 4) + 0x10);
        next = i + a4;
        switch (a3 & 7) {
        case 1:
            func_00378758(cur, a0, 0, a2, a5, 0x7F, var, 0xFF);
            break;
        case 2:
            func_00378758(cur, a0, 1, a2, a5, 0x7F, var, 0xFF);
            break;
        case 4:
            cur[2] += a5 * 0.5f;
            n = i;
            do {
                func_00378758(cur, a0, n % 10 + 2, a2, a5, 0x7F, var, 0xFF);
                n /= 10;
                func_0032D5C8(cur, cur, vec);
            } while (n != 0);
            break;
        }
        qcopy(prev, route + (i << 4) + 0x10);
    }
}
