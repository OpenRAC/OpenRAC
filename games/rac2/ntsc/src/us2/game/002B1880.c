#include "include_asm.h"

void func_US2_002B1880(int first, int second)
{
    extern int func_00288FB8(int);
    extern void func_0029CC98(int, int);
    func_0029CC98(func_00288FB8(first), second);
}

extern void func_0029CC50(int, int, int);

void func_US2_002B18B0(int first, int second, int third) {
    func_0029CC50(first, second, third);
}

