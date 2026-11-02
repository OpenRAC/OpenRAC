/* Alternative reconstruction retained from PR #361. Not integrated or qualified;
 * compare with the primary near miss before reuse. */
/* func_L04_003F9C60: 620 of 624 bytes; first loop and prologue match retail exactly
 * (including the por/sq zeroing and callee-saved assignment). The only remaining difference is
 * in the second loop: retail zeroes the 16-byte vector at sp+0 with "por $v1,$zero,$zero;
 * sq $v1,0($sp)" (after the second rand call), mine emits "sq $zero,0($sp)" (one instruction
 * shorter, scheduled differently). The union with a TI member at function scope (shared slot
 * for loop 1's copy and loop 2's vector), separate counter j, and zeroing after the sign
 * call were what got everything else to match. Tried: aligned struct init (memset call),
 * struct = zero4 extern (data copy), zero via copy from v.q (lq), store order of f[2]/f[3].
 */
#include "common.h"
typedef int Q16_B0e5d8f83 __attribute__((mode(TI)));
typedef union {
    Q16_B0e5d8f83 q;
    float f[4];
} Vec4_003F9C60;

extern void func_003530F0(void *);
extern float func_0033BB18(float, float);
extern int func_0033BAC0(int, int);
extern int func_0033BA80(int);
extern void func_0030F090(void *, void *, float);
extern void func_00358AE8(void *, void *, float, int, int);
extern u8 *func_00357D68(void *, int, void *, int, int, float, float, float, float);

void func_L04_003F9C60(u8 *a0, int a1) {
    void **slot = (void **)(*(u8 **)(a0 + 0x68) + 0x1C);
    int i;
    int j;
    int color;
    int sign;
    u8 *p;
    u8 *q;
    Vec4_003F9C60 w;
    Vec4_003F9C60 v;
    for (i = 0x10; i >= 0; i--) {
        if (*slot != 0) {
            func_003530F0(*slot);
        }
        *slot = 0;
        slot++;
    }
    if (a1 != 0) {
        for (i = 4; i >= 0; i--) {
            v.q = 0;
            v.f[0] = func_0033BB18(-1.0f, 1.0f);
            v.f[1] = func_0033BB18(-1.0f, 1.0f);
            v.f[2] = func_0033BB18(-1.0f, 1.0f);
            w.q = v.q;
            func_0030F090(&v, &w, func_0033BB18(0.050000004f, 0.10000001f));
            func_00358AE8(a0 + 0x10, &v, 35000.0f, 0x7F2F4F6F, func_0033BAC0(10, 15));
        }
    }
    for (j = 2; j >= 0; j--) {
        color = (func_0033BAC0(0x20, 0x80) << 24) | 0x606060;
        sign = 1;
        if (func_0033BA80(2) != 0) {
            sign = -1;
        }
        w.q = 0;
        w.f[2] = 1.0f;
        w.f[3] = 1.0f;
        w.f[2] = func_0033BB18(0.01f, 0.025f);
        p = func_00357D68(a0 + 0x10, sign, &w, color, 0, 0.2f, 1.0f, 1.02f, 140000.0f);
        if (p != 0) {
            q = p + 0x20;
            *(s16 *)(p + 0xA) = func_0033BAC0(0x32, 0x50);
            q[0xA] = (u32)color >> 24;
            *(int *)(q + 4) = 2;
            q[0xB] = p[0xA];
            if (func_0033BA80(2) != 0) {
                p[3] = 0x44;
            }
        }
    }
}
