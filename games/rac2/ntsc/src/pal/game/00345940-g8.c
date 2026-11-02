#include "include_asm.h"
extern void func_00336320(void *a0);
extern void func_00336A78(void *a0);
extern void func_00342BE8(void *a0);
extern int func_00342BC0(void *a0);
extern int func_00288FB8(int);
extern void func_00336A38(void *a0, int a1);
extern unsigned char D_00139568[];
extern char D_00239BA0[];

void func_PAL_00345940(char *self)
{
    int r;
    char *rec;

    if (*(int *)(self + 0x2C0) != 0) {
        func_00336320(self);
        func_00336320(self + 0x4C);
        func_00336320(self + 0x98);
        func_00336A78(self + 0x1B0);
        func_00342BE8(self + 0x2C8);
        r = func_00342BC0(self + 0x2C8);
        rec = D_00239BA0 + D_00139568[r] * 224;
        func_00336A38(self + 0x208, func_00288FB8(*(int *)(rec + 8)));
        func_00336A78(self + 0x208);
        func_00336A78(self + 0x260);
    }
}
