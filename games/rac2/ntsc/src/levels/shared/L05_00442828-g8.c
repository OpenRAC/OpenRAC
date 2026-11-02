#include "common.h"
extern int D_L05_001B1B20 __attribute__((sda));
extern int D_L05_001B1B24 __attribute__((sda));
extern char D_L05_001B3610[] __attribute__((aligned(16))) MACRO_ADDR;
extern char D_L05_001B3620[] __attribute__((aligned(16))) MACRO_ADDR;

void func_L05_00442828(char *a0, char *a1, int a2)
{
    D_L05_001B1B20 = 1;
    D_L05_001B1B24 = a2;
    qcopy(D_L05_001B3610, a1);
    qcopy(D_L05_001B3620, a0);
}
