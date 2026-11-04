/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L08_0040FAE8: 612 of 628 bytes (16 over), 0 of 1 place. The control flow decoded from retail
 * (collision probe, pushback copy from the 0x1BFF60 vector pair, clamp-and-notify tail with the
 * 2.0 / 1021.0 bounds and func_0031D540) is complete; what differs is register numbering
 * (retail o=$s2, o+0x10=$s0, p=$s1; mine o=$s1, o+0x10=$s2, p=$s0) and retail re-forms the
 * 0x1BFF70 address (lui/addiu -0x90 plus a second -0x30/+0x38 form) three times where mine
 * folds it, which accounts for the missing instructions. Tried: pointer q = D[1] with q - 4,
 * declaration order of the two pointers.
 */
#include "common.h"
extern void func_002F9DF0(void *, void *, void *);
extern int func_002EA300(void *, void *, int, void *, int);
extern int func_002EB360(void *, int, void *, float);
extern void func_002FA228(void *, void *, void *);
extern void func_002F9E60(void *, void *, float);
extern float func_002F9F20(void *);
extern float func_002FADE0(float, float);
extern void func_0031D540(void *);
extern float D_L08_001BFF60[][4] __attribute__((aligned(16)));

void func_L08_0040FAE8(u8 *o) {
    float v[4] __attribute__((aligned(16)));
    u8 *s0 = o + 0x10;
    u8 *s1 = *(u8 **)(o + 0x68);
    u8 *a0;
    float *q;
    int r;

    func_002F9DF0(v, s0, s1);
    r = func_002EA300(s0, v, 0, *(void **)(s1 + 0x18), 0);
    if (r == 0) {
        v[2] += 0.3f;
        r = func_002EB360(v, 0, *(void **)(s1 + 0x18), 0.3f);
        v[2] -= 0.3f;
    } else {
        q = D_L08_001BFF60[1];
        qcopy(q, q - 4);
        q[2] += 0.3f;
    }
    if (r != 0) {
        qcopy(s0, D_L08_001BFF60[1]);
        *(float *)(o + 0x18) -= 0.3f;
        func_002FA228(s1, s1, D_L08_001BFF60[2]);
        func_002F9E60(s1, s1, 0.75f);
        if (func_002F9F20(s1) < 0.01f) {
            func_0031D540(o);
            return;
        }
    } else {
        qcopy(s0, v);
    }
    *(float *)(s1 + 8) -= *(float *)(s1 + 0x10);
    *(float *)(o + 0xF4) = func_002FADE0(*(float *)(o + 0xF4), *(float *)(s1 + 0x14));
    a0 = *(u8 **)(s1 + 0x1C);
    if (a0 == 0 || *(s16 *)(a0 + 0xAA) != 0x326 || a0[0x20] == 0xFE || a0[0x20] == 0xFD
        || *(float *)(o + 0x10) < 2.0f || *(float *)(o + 0x10) > 1021.0f
        || *(float *)(o + 0x14) < 2.0f || *(float *)(o + 0x14) > 1021.0f
        || *(float *)(o + 0x18) < 2.0f || *(float *)(o + 0x18) > 1021.0f) {
        func_0031D540(o);
        return;
    }
    qcopy(a0 + 0x10, s0);
    qcopy(a0 + 0xF0, o + 0xF0);
}
