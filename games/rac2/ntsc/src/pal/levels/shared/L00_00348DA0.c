#include "include_asm.h"

extern void func_0027C540_2(long);
extern void func_L00_0030D210(void);
extern void func_0027C660_2(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
int func_PAL_L00_00348DA0(void)
{
  func_0027C540_2(0);
  func_L00_0030D210();
  func_0027C660_2();
  return 0;
}

