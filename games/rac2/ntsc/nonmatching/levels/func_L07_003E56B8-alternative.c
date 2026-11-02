/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L07_003E56B8: same size as retail (600 bytes), 0 of 1 place. Needs -G8 (reads two $gp
 * floats; it lives in L07_00389EB8-g8.c when built). Everything matches except float register
 * numbering and the order of the init stores: retail names the 1/3600 constant $f1, the two
 * gp-scaled values $f3/$f2 and the 0.5 constant $f1 again, and emits the stores in source order
 * (0x98, 0x80, 0x8C, 0xAC, 0x120, 0x144, 0x148, 0x158, 0x84, 0xA8, 0x124, 0x140, 0x30); mine
 * reorders the float stores and numbers the constants differently. Tried: temps for the two
 * scaled values (shared t for 0x140/0x144 is right), declaration order, inline second product,
 * store order of 0x2E/0xC7. D_L07_001B0AB0/0AB4 are my guess for the two gp symbols (gp+0x1AB0/4).
 */
#include "common.h"
extern int func_002E5470(float);
extern float func_002E44D0(void *, void *);
extern void func_003109C8(void *, int, int);
extern void func_00312D18(void *);
extern int func_00355488(int, int, void *);
extern int func_0031D208(void *, void *, int);
extern void func_0031FFD0(void *, int, int, int, float, float);
extern void func_L07_003E5AC8(void *);
extern void func_L07_003E5D80(void *);
extern void func_L07_003E6148(void *);
extern u8 D_00186660[];
extern u8 D_L07_001B9200[];
extern float D_L07_001B0AB0 __attribute__((sda));
extern float D_L07_001B0AB4 __attribute__((sda));

void func_L07_003E56B8(u8 *o)
{
    u8 *p = *(u8 **)(o + 0x68);
    u8 *e;

    if (o[0x20] == 0) {
        float t2;
        float t;
        p[0x48] = func_002E5470(15.0f);
        p[0x4A] = func_002E5470(2.4f);
        p[0x60] = 7;
        p[0x2E] = 1;
        p[0xC7] = 1;
        t = D_L07_001B0AB0 * 0.00027777778f;
        t2 = D_L07_001B0AB4 * 0.016666668f;
        *(int *)(p + 0x98) = 3;
        *(float *)(p + 0x80) = 0.5f;
        *(float *)(p + 0x8C) = 0.15f;
        *(float *)(p + 0xAC) = 1.0f;
        *(float *)(p + 0x120) = 0.45f;
        *(float *)(p + 0x144) = t;
        *(float *)(p + 0x148) = t2;
        *(int *)(p + 0x158) |= 0x15;
        *(int *)(p + 0x84) = 0;
        *(float *)(p + 0xA8) = 1.0f;
        *(int *)(p + 0x124) = 0;
        *(float *)(p + 0x140) = t;
        *(int *)(p + 0x30) = 0;
        qcopy(p + 0x220, o + 0x10);
        if (*(s16 *)(p + 0x218) != 0) {
            func_003109C8(o, 1, 0);
        } else {
            *(int *)(o + 0x98) = 0;
            *(u16 *)(o + 0x34) = (*(u16 *)(o + 0x34) | 0x41) & 0xEFFF;
            func_003109C8(o, 8, 4);
        }
    }
    if (o[0x31] != 0) {
        if (func_002E44D0(o + 0x10, D_L07_001B9200) < 24.0f) {
            func_00312D18(o);
            o[0x6F] = 0x16;
        }
    }
    if (o[0x95] != 4) {
        e = D_00186660 + *(int *)(p + 0x264) * 0x70;
        if (*(u8 **)(e + 0x88) != o || e[0x74] == 0) {
            *(int *)(p + 0x264) = func_00355488(1, 4, o);
        }
    }
    if (func_0031D208(o, p + 0x50, 0) >= 2 || p[0x2E] == 2) {
        func_L07_003E5AC8(o);
        func_L07_003E5D80(o);
    }
    func_0031FFD0(o, *(int *)(p + 0x214), 0, 0, 0.0f, 0.0f);
    if (*(int *)(p + 0x260) != 0 && o[0x20] != 8) {
        func_L07_003E6148(o);
    }
}
