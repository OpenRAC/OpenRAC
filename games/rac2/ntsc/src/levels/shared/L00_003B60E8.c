#include "include_asm.h"
typedef struct OBJ4 {
    char pad0[8];
    int f8;
    int fC;
} OBJ4;

extern void func_L00_00322EC8(void *);

void func_L00_003B60E8(OBJ4 *a0)
{
    char *base = (char *)a0 + 0x20;
    char *s = base + a0->f8;
    int *w = (int *)(base + a0->fC);
    int i;

    for (i = 0; i < 25; i++) {
        if (w[i] != 0)
            func_L00_00322EC8(s + i * 0x150);
    }
}
