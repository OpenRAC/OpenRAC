#include "include_asm.h"

/* func_00307298 - 148 bytes, 1 place (the boot); the G8 twin of func_00304AD0.
 * The qcopy barrier reproduces retail's first-call argument setup. */

typedef int TI128 __attribute__((mode(TI)));

extern float D_001AEC50 __attribute__((sda));
extern void func_00282CC0(void *a, void *b, void *c);
extern void func_002FE880(void *a, void *b, float c);
extern int func_002761F8(void *a, void *b, int c, void *d, int e);

void func_00307298(void *obj)
{
    void *s2 = obj;
    void *s0 = (char *)s2 + 0x10;
    void *s1 = *(void **)((char *)s2 + 0x68);
    TI128 local;

    *(TI128 *)&local = *(TI128 *)s0;
    func_00282CC0(s0, s0, s1);
    func_002FE880(s0, s1, D_001AEC50 * 2.7777778e-05f);
    if (func_002761F8(&local, s0, 0x10, s2, 0) != 0)
        *(unsigned char *)((char *)s2 + 0x20) = 8;
}
