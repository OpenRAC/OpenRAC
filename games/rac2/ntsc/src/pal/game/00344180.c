#include "include_asm.h"

extern void func_00348068(unsigned char *owner, float *records);

/* func_00342118 is in 00341C20-g8.c (it reaches small data through $gp). */

void func_PAL_003443E8(unsigned char *owner, float *records)
{
    *(float **)(owner + 0x260) = records;
    func_00348068(owner + 0x188, records);
}

