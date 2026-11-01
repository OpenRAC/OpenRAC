#include "include_asm.h"

/* func_L23_003C01F8 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int TI16 __attribute__((mode(TI)));

extern float D_L23_001AFA10 __attribute__((sda));

extern TI16 D_L23_001C0AA0;

extern void func_L23_003BFFC0(char *arg0, int arg1, int arg2, int arg3, int arg4, float arg5);

extern void func_L23_00418650(TI16 *, float);

extern int func_002761F8(TI16 *, TI16 *, int, int, int);

void func_L23_003C01F8(char *arg0, int arg1) {
    TI16 a;
    TI16 b;
    TI16 *p;

    if ((unsigned char)(*(unsigned char *)(arg0 + 0x43) - 5) >= 2 && *(unsigned char *)(arg0 + 0x20) != 2)
        func_L23_003BFFC0(arg0, 5, 0, 6, 1, D_L23_001AFA10);
    if (arg1 != 0) {
        TI16 *pos = (TI16 *)(arg0 + 0x10);

        /* b's address is formed between the two copies, where retail has it. */
        p = &a;
        a = *pos;
        arg0 = (char *)&b;
        b = *pos;
        func_L23_00418650((TI16 *)arg0, -2.0f);
        func_L23_00418650(p, 0.1f);
        if (func_002761F8((TI16 *)arg0, p, 0x12, 0, 0) != 0)
            *pos = D_L23_001C0AA0;
    }
}
