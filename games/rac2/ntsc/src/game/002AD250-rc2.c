/* func_002AD250 in its own unit: it copies 128 bits, and the retail's copy
   keeps both addresses in registers (`lq $v0,0($sX)` / `sq $v0,0($v1)`).
   That is the compiler's opt-in grouped copy, so config/cflags.txt gives this
   file "-O2 -G0 -ffunction-sections -frac2-qcopy"; the flag changes every
   TImode copy in a unit, so the unit holds this function alone. */

#include "include_asm.h"

/* The 128-bit integer mode is what reaches retail's lq/sq copies. */
typedef int TI128 __attribute__((mode(TI)));

/* The matrix the engine copies into place. */
typedef struct { char pad[64]; } Matrix64_002AA800;

extern int func_002AD0B0(unsigned char *a);
extern void func_00283410(char *dst, char *src);
extern void func_00283698(char *dst, char *src);
extern void func_00283098(char *a, char *b, char *c);

/* Puts the matrix of the object the second argument names into the third
   argument, and its two 128-bit fields into the fourth; without the object's
   flag the fields stay as they are. */
int func_002AD250(void *unused, void *a1, void *a2, void *a3)
{
    Matrix64_002AA800 m0, m1, m2;
    char *p = (char *)func_002AD0B0((unsigned char *)a1);

    if (p == 0) {
        *(TI128 *)a3 = *(TI128 *)a2;
        return 0;
    }
    func_00283410((char *)&m0, p);
    if (*(int *)(p + 0x3C) & 2) {
        func_00283410((char *)&m2, p + 0x20);
        func_00283698((char *)&m1, (char *)&m2);
        func_00283098((char *)a3, (char *)a2, (char *)&m1);
        func_00283098((char *)a3, (char *)a3, (char *)a1 + 0xC0);
    } else {
        func_00283098((char *)a3, (char *)a2, (char *)&m0);
    }
    return 1;
}
