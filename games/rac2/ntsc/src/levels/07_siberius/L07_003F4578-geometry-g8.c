#include "common.h"
extern u8 *D_L07_001B22DC __attribute__((sda));
extern void func_002AD5F8(u8 *,u8 *,u8 *,u8 *,u8 *,u8 *);
void func_L07_0040F6B8(u8 *a0)
{
    u8 *p = *(u8 **)(a0 + 0x68);
    u8 *b = p + 0x160;
    u8 *c = p + 0x120;
    s32 *ids = (s32 *)(p + 0xA8);
    s32 i = 3;

    while (i >= 0) {
        if (*ids != -1) {
            u8 *e = D_L07_001B22DC + (*ids << 8);
            func_002AD5F8(e, a0, e + 0x10, e + 0xF0, c, b);
        }
        b += 0x10;
        c += 0x10;
        i--;
        ids++;
    }
}
