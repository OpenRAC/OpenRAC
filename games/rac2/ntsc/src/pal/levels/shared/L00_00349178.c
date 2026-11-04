#include "include_asm.h"

unsigned long long func_L00_00346A80(void);
extern void func_0027C540_9(long);
extern void func_L00_0030D2B0(void);
extern void func_0027C660_9(void);

/* Run the three measured setup calls with an explicit long zero, then return the defined integer zero. */
unsigned long long func_PAL_L00_00349178(void)
{
  func_0027C540_9(0);
  func_L00_0030D2B0();
  func_0027C660_9();
  return 0;
}

