#include "include_asm.h"

extern int D_L00_001A904C_3 __attribute__((sda));
extern int func_00344378(int x);

/* When the global is set, derives a pointer from it plus a fixed offset, runs it through a callee and stores the result into a field of the argument. */
int func_US2_L00_003480A0(int *obj)
{
    if (D_L00_001A904C_3 != 0)
        obj[13] = func_00344378(D_L00_001A904C_3 + 0x3A000);
    return 0;
}

extern void func_0027C540_7(long);
extern void func_L00_0030D528(void);
extern void func_0027C660_7(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
long long func_US2_L00_003480E8(void)
{
    func_0027C540_7(0LL);
    func_L00_0030D528();
    func_0027C660_7();
    return 0LL;
}

