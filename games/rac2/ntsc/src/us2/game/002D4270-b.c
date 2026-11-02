#include "include_asm.h"
extern char *D_001A8CFC[] __attribute__((sda));
extern float ref_L00_002D33C0_00[];
extern void ref_L00_002D33C0_01(unsigned char *, float *);
extern char *D_001A8CFC[] __attribute__((sda));
extern char *D_001A8CFC[] __attribute__((sda));

int func_US2_002D4370(void)
{
    if (D_001A8CFC[0] != 0)
        ref_L00_002D33C0_01((unsigned char *)(D_001A8CFC[0] + 0x3F3F0), ref_L00_002D33C0_00);
    return 0;
}
