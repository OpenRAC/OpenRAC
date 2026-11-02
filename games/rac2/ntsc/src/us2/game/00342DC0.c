#include "include_asm.h"

extern void func_00348068(unsigned char *owner, float *records);

void func_US2_00342DC0(unsigned char *owner, float *records)
{
    func_00348068(owner + 0x10, records);
    *(float **)(owner + 0xE8) = records;
}

