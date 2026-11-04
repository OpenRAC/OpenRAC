/* func_003001E0 in its own unit (flags in config/cflags.txt: -O2 -G8 -ffunction-sections -frac2-qcopy). */
#include "include_asm.h"


typedef int TI128 __attribute__((mode(TI)));

extern int ref_L00_003001E0_00 __attribute__((sda));

extern unsigned char D_0018C0B0[];

extern void func_002AD330(void *, void *, void *, void *);

extern void func_00282CC0(void *, void *, void *);

void func_003001E0(void *a0, void *a1, void *a2)
{
    TI128 buf;
    int *state = ref_L00_003001E0_00;

    if (state != 0) {
        func_002AD330(*(int *)D_0018C0B0, state, a2, &buf);
        func_00282CC0(a0, a1, &buf);
    } else {
        *(TI128 *)a0 = *(TI128 *)a1;
    }
}
