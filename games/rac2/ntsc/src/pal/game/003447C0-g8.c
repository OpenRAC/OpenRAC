#include "include_asm.h"
extern void func_00336320(int *);
extern void func_003425B8(int *);
extern void func_00342648(int);

void func_PAL_003447C0(int *p)
{
    if (p[92] != 0) {
        func_00336320(p);
        func_00336320(p + 19);
        func_003425B8(p);
        func_00342648((int)p);
        func_00342668(p);
    }
}
