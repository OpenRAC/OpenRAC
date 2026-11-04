#include "include_asm.h"
extern float func_002A7878_2(float a, float b);
extern unsigned int func_002A7820_1(int a, int b);
extern void func_002FC560(unsigned int reg, unsigned long value);
extern float func_002A7940(void);
extern void func_00281EB8(char *a, int b, unsigned int c, int d, float e, float f,
                          float g);
extern void func_L00_003BBF48(char *a0);
extern void func_L00_003BCDC0(char *a0);

/* Opens the block, lays seven oriented sprites over it from two helper draws and the shared
   colour word, then runs the two slot-table passes and closes the block. */
void func_PAL_L00_003BF2F8(char *a0)
{
    char *ctx;

    ctx = *(char **)(a0 + 0x68);
    func_002FC560(0x47, 0x00053001);

    func_00281EB8(a0 + 0x10, 0x2C,
                  (func_002A7820_1(0x80, 0xC8) << 24) | 0x72FF, 1,
                  func_002A7878_2(0.38500002f, 0.415f) * *(float *)(ctx + 0x4C),
                  -0.01f, func_002A7940());
    func_00281EB8(a0 + 0x10, 0x2D,
                  (func_002A7820_1(4, 0x28) << 24) | 0x00DDB973, 1,
                  *(float *)(ctx + 0x4C) * 1.225f, 0.0f, func_002A7940());
    func_00281EB8(a0 + 0x10, 0x29,
                  (func_002A7820_1(4, 0x28) << 24) | 0x00DDB973, 1,
                  *(float *)(ctx + 0x4C) * 0.6f, 0.0f, func_002A7940());
    func_00281EB8(a0 + 0x10, 0x2A,
                  (func_002A7820_1(4, 0x2D) << 24) | 0x00DDB973, 1,
                  func_002A7878_2(0.975f, 1.025f) * *(float *)(ctx + 0x4C),
                  0.0f, func_002A7940());
    func_00281EB8(a0 + 0x10, 0x2A,
                  (func_002A7820_1(4, 0x2D) << 24) | 0x00DDB973, 1,
                  func_002A7878_2(0.975f, 1.025f) * *(float *)(ctx + 0x4C),
                  0.0f, func_002A7940());
    func_L00_003BBF48(a0);
    func_L00_003BCDC0(a0);
    func_002FC560(0x47, 0x0005360B);
}
