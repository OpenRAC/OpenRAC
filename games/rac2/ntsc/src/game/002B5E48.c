#include "include_asm.h"

/* func_002B5E48 is a unit of its own: it needs the opt-in quadword-copy profile and its
   scheduling barrier (-frac2-qcopy -frac2-qcopy-barrier, config/cflags.txt), which give
   retail's grouped `lq`/`sq` with register addresses and keep the instructions around the
   copy on their own side of it. */

typedef int TI128_002B5E48 __attribute__((mode(TI)));
typedef union { TI128_002B5E48 q; float f[4]; } Vec4Q_002B5E48;

extern int func_002C9218(int a0, void *a1, void *a2, float f);
extern void func_00282CF0(void *, void *, void *);
extern void func_00282F20(char *, char *, float);

int func_002B5E48(char *obj, char *moby)
{
    Vec4Q_002B5E48 v;
    char *s2;
    int id;

    id = *(int *)(moby + 0x3C);
    if (id == -1) return 0;
    if (*(unsigned int *)(moby + 0x9C) & 1) return 0;
    s2 = obj + 0x10;
    v.q = *(TI128_002B5E48 *)s2;
    if (func_002C9218(id, &v, s2, *(float *)moby) != 0) {
        func_00282CF0(moby + 0x60, &v, s2);
        func_00282F20(moby + 0x60, moby + 0x60, *(float *)moby);
        *(unsigned int *)(moby + 0x94) |= 0x10;
        return 1;
    }
    return 0;
}
