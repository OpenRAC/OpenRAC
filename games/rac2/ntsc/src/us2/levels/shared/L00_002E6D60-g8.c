#include "include_asm.h"
extern void func_0027BD70(void);
extern unsigned int D_L00_001B18D4;

void func_US2_L00_002E6D60(void) {
 D_L00_001B18D4 = 0x100FF;
 func_0027BD70();
}

extern void func_00284F40(int);
extern void func_0027DDC0(int, int, int, int);

void func_US2_L00_002E6E18(void) {
 func_00284F40(0);
 func_0027DDC0(0, 0, 0, 0x80);
 D_L00_001B18D4 = 0xF0;
 func_0027BD70();
}
