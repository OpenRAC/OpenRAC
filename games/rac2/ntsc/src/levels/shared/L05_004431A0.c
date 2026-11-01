#include "include_asm.h"
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));

extern int D_001A79F0 MACRO_ADDR;

extern int D_001A79F0_alt MACRO_ADDR;

extern TI128 D_L05_001B35D0 MACRO_ADDR;

extern TI128 D_L05_001B35E0 MACRO_ADDR;

extern TI128 D_L05_001B35F0 MACRO_ADDR;

extern float ref_L05_004431A0_00(float);

extern void ref_L05_004431A0_01(void *, void *, float);

extern void ref_L05_004431A0_02(void *, void *, void *);

void func_L05_004431A0(void *a0, float *a1, float *a2)
{
    void *s0 = a0;
    float *s1 = a1;
    float *s2 = a2;
    TI128 scratch;
    float f20;

    if (D_001A79F0 == 20)
        goto done;
    f20 = 1e-5f;
    if (f20 < ref_L05_004431A0_00(*s1)) {
        ref_L05_004431A0_01(&scratch, (char *)s0 + 0x20, *s1);
        ref_L05_004431A0_02(s0, s0, &scratch);
        ref_L05_004431A0_02((char *)s0 + 0x10, (char *)s0 + 0x10, &scratch);
    }
    if (D_001A79F0_alt == 20 || D_001A79F0_alt == 15)
        goto done;
    if (f20 < ref_L05_004431A0_00(*s2)) {
        ref_L05_004431A0_01(&scratch, (char *)s0 + 0x10, *s2);
        ref_L05_004431A0_02(s0, s0, &scratch);
        ref_L05_004431A0_02((char *)s0 + 0x20, (char *)s0 + 0x20, &scratch);
    }
done:
    qcopy(&D_L05_001B35D0, a0);
    qcopy(&D_L05_001B35E0, (char *)s0 + 0x10);
    qcopy(&D_L05_001B35F0, (char *)s0 + 0x20);
}
