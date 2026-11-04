#include "include_asm.h"

extern void func_0027C540(int a);
extern void func_0027C660(void);
extern void func_L00_0030DB88(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
int func_US2_L00_0034FD48(void)
{
    func_0027C540(0);
    func_L00_0030DB88();
    func_0027C660();
    return 0;
}

extern int D_00138344;
extern void func_L00_0030D478(int a);

int func_US2_L00_0034FDD0(void)
{
    func_L00_0030D478(D_00138344);
    return 0;
}

