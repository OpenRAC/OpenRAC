/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L07_003E6FE8: 604 of 600 bytes (one instruction over), 0 of 1 place. Control flow, calls,
 * constants and the three stack vectors match. Differences: callee-saved numbering (retail
 * o=$s2, p=$s3, o+0x10 and the o+0xF0/o+0xC0 temporaries in $s1/$s0, $s4 only in the second
 * path; mine o=$s1, p=$s2, base=$s3, pos=$s4) and retail tests the 0x1BF640 block's +0x18
 * with "lw $a0,0x18($s0); beql $a0,$zero" (delay = the epilogue's ld) where mine uses
 * "lw $v1; beqz" plus an extra nop. Tried: separate variables for the two paths' temporaries
 * (no effect on numbering).
 */
#include "common.h"
extern int func_002D4510(void *, void *, int, void *, int);
extern u8 *func_00316FD8(void);
extern void func_002E4340(void *, void *, void *);
extern void func_0031B090(void *, void *, void *, int);
extern void func_00317520(void *, void *, void *, void *, void *, void *);
extern void func_00317398(void *, void *, void *, void *, void *, void *);
extern void func_00317008(void *, void *, void *, void *, void *, void *);
extern void func_002E4B70(void *, void *);
extern void func_0031B710(void *, void *, int);
extern void func_003152C0(void *, void *);
extern void func_002E4398(float, void *, void *);
extern void func_002E47D0(void *, void *, void *);
extern u8 D_L07_001BF640[];

void func_L07_003E6FE8(u8 *o) {
    u8 *p = *(u8 **)(o + 0x68);
    float a[4] __attribute__((aligned(16)));
    float b[4] __attribute__((aligned(16)));
    float c[4] __attribute__((aligned(16)));
    u8 *s0;
    u8 *s1;
    u8 *s4;
    u8 *pos;
    u8 *base;

    if (*(u8 **)(p + 0x338) == 0) {
        pos = o + 0x10;
        qcopy(a, pos);
        qcopy(b, pos);
        a[2] += 2.0f;
        b[2] -= 3.0f;
        if (func_002D4510(a, b, 0, o, 0) != 0) {
            base = D_L07_001BF640;
            if (*(u8 **)(base + 0x18) != 0) {
                if (*(int *)(func_00316FD8() + 0x3C) & 2) {
                    *(u8 **)(p + 0x338) = *(u8 **)(base + 0x18);
                    func_002E4340(c, pos, *(u8 **)(base + 0x18) + 0x10);
                    func_0031B090(*(u8 **)(p + 0x338), c, c, 0);
                    *(float *)(p + 0x33C) = c[2] - 1.0f;
                    func_00317520(o, *(u8 **)(p + 0x338), pos, o + 0xF0, p + 0x310, p + 0x320);
                }
                *(int *)(p + 0x88) |= 0x100;
            }
        }
        return;
    }
    if (o[0x20] == 9 || o[0x20] == 0xB) {
        s0 = o + 0x10;
        s1 = o + 0xF0;
        func_00317398(o, *(u8 **)(p + 0x338), p + 0x310, p + 0x320, s0, s1);
        s4 = s0;
    } else {
        s0 = o + 0xC0;
        s1 = o + 0xF0;
        func_00317008(o, *(u8 **)(p + 0x338), s0, s1, s0, s1);
        s4 = s0;
    }
    s0 = o + 0xC0;
    func_002E4B70(s0, s1);
    func_0031B710(s0, *(u8 **)(p + 0x338) + 0xE0, 0);
    func_003152C0(s0, s1);
    if (*(u16 *)(o + 0x34) & 0x8000) {
        func_002E4398(-1.0f, o + 0xD0, o + 0xD0);
    }
    if (o[0x95] == 3) {
        func_002E4340(a, s4, *(u8 **)(p + 0x338) + 0x10);
        func_0031B090(*(u8 **)(p + 0x338), a, a, 0);
        a[2] = *(float *)(p + 0x33C);
        func_002E47D0(a, a, *(u8 **)(p + 0x338) + 0xC0);
        *(float *)(p + 0x180) = a[2] + *(float *)(*(u8 **)(p + 0x338) + 0x18);
    }
}
