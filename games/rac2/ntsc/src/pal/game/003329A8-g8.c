#include "include_asm.h"
extern void func_002FC808(void);
extern void func_00330EC0(void *);
extern void func_002FDBD8(void *, void *, int, float);
extern float ref_L00_00330E48_00 __attribute__((sda));

void func_PAL_003329A8(char *owner)
{
    char *data;
    func_002FC808();
    data = *(char **)(owner + 0x68);
    if (*(float *)(data + 0x64) < ref_L00_00330E48_00)
        func_00330EC0(owner);
    if (*(int *)(data + 0x78))
        func_002FDBD8(data + 0x30, data + 0x40, 0, 0.2f);
}
