#include "include_asm.h"


typedef int TI128 __attribute__((mode(TI)));
extern float D_L00_001AF7A4 __attribute__((sda));
extern void func_00283098(void *, void *, void *);
extern void func_00282CC0(void *, void *, void *);

/* Same body as func_L00_003ACC88, with this function's own callees. */
extern float ref_L00_0040B2C0_00 __attribute__((sda));
void func_L00_0040B2C0(char *a0, char *a1)
{
    float other[4];
    float local[4];
    TI128 zero = 0;

    *(TI128 *)local = zero;
    local[0] = ref_L00_0040B2C0_00;
    local[3] = 1.0f;
    *(TI128 *)other = *(TI128 *)local;
    func_00283098(other, other, a0 + 0xC0);
    func_00282CC0(a1, other, a0 + 0x10);
}
