#include "common.h"
extern void func_002A91F0(void *a0, float *a1);

void func_L11_0044C180(char *a0)
{
    char *s0 = *(char **)(a0 + 0x68);
    char *p;
    char *dst;
    char *src;

    *(s32 *)(s0 + 0x4B8) = 0;
    *(s32 *)(s0 + 0x4B4) = (*(s32 *)(s0 + 0x4B4) + 1) % 4;
    if (*(s32 *)(s0 + 0x4B0) < 4)
        *(s32 *)(s0 + 0x4B0) = *(s32 *)(s0 + 0x4B0) + 1;
    func_002A91F0(((*(s32 *)(s0 + 0x4B4) << 4) + 0x370) + s0, (float *)(a0 + 0xF0));
    p = *(char **)(s0 + 0x4A4);
    if (p != 0) {
        src = p + 0x10;
        dst = (char *)((*(s32 *)(s0 + 0x4B4) << 4) + (u32)s0 + 0x270);
        qcopy(dst, src);
    }
}
