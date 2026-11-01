#include "include_asm.h"

extern void func_0027C540(int a);
extern void func_0027C660(void);
extern void func_L00_0030D448(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
int func_US2_L00_0034FDF8(void)
{
    func_0027C540(0);
    func_L00_0030D448();
    func_0027C660();
    return 0;
}

