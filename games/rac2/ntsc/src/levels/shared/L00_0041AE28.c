#include "include_asm.h"
extern void func_002FC560(unsigned int reg, unsigned long value);
extern void func_L00_0041B1B8(char *, unsigned int, int, float);
extern void func_L00_0041AFD8(char *, void *, unsigned int, float);
extern void func_L00_0041BCC8(char *);

void func_L00_0041AE28(char *o)
{
    char *st = *(char **)(o + 0x68);

    func_002FC560(0x47, 0x53001);
    func_L00_0041B1B8(o, 0x7F7F7F7F, 0x4B, 0.05f);
    if (*(float *)(st + 0x50) != 0.0f) {
        func_L00_0041AFD8(o, st + 0x10, 0x7F10307F, 0.25f);
        func_L00_0041AFD8(o, st + 0x10, 0x3F08183F, 0.5f);
    }
    func_L00_0041BCC8(o);
    func_002FC560(0x47, 0x5360B);
}
