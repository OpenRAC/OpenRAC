#include "include_asm.h"
extern char *D_001B2514 __attribute__((section(".sdata")));
extern char *D_001B251C __attribute__((section(".sdata")));
extern char *D_L00_00226700[];
extern void func_00283098(void *, void *, void *);
extern float func_00282DF0(void *);
extern float func_L00_002EE050(float, float);
extern void func_00282D60(void *, void *, float);
extern void func_00282CC0(void *, void *, void *);
void func_L00_0036AE00(int index, int flag)
{
    char *rec = D_001B2514 + index * 32;
    char *blk = D_001B251C + index * 64;
    float a, m;
    func_00283098(rec, D_L00_00226700[(unsigned char)rec[0x1D]] + 0x50, blk);
    a = func_00282DF0(blk);
    a = func_L00_002EE050(a, func_00282DF0(blk + 0x10));
    m = func_L00_002EE050(a, func_00282DF0(blk + 0x20));
    *(float *)(rec + 0xC) = *(float *)(D_L00_00226700[(unsigned char)rec[0x1D]] + 0x5C) * m;
    func_00282D60(rec, rec, *(float *)(blk + 0x3C));
    func_00282CC0(rec, rec, blk + 0x30);
    *(float *)(blk + 0xC) = 1.0f / func_00282DF0(blk);
    *(float *)(blk + 0x1C) = 1.0f / func_00282DF0(blk + 0x10);
    *(float *)(blk + 0x2C) = 1.0f / func_00282DF0(blk + 0x20);
    if (flag != 0)
        rec[0x1E] = 1;
}
