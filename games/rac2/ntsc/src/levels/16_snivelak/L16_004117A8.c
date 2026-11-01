#include "common.h"

extern u8 *D_L16_001B2B1C[] __attribute__((sda));

extern u8 *D_L16_001B2B20[] __attribute__((sda));

extern u8 *func_002AB1F0(u8 *);

extern void func_0029F338(void *);

/* Walks the level's entity list and reports the classes that need a second pass. */
void func_L16_004117A8(u8 *arg0)
{
    u8 *s0;
    u8 *r;
    u32 id;
    s16 cls;

    s0 = D_L16_001B2B1C[0];
    if (D_L16_001B2B20[0] < s0)
        return;
    do {
        id = *(u16 *)(s0 + 0xAA);
        if (id - 0xE71 < 3 || (s16)id == 0xC87 || (s16)id == 0xC86
            || (s16)id == 0x113A || (s16)id == 0x1162 || (s16)id == 0x7FB) {
            func_0029F338(s0);
        }
        cls = *(s16 *)(s0 + 0xAA);
        if (cls == 0xF35 || cls == 0xEA3 || cls == 0xEA4 || cls == 0xEBB) {
            r = func_002AB1F0(s0);
            if (r != 0) {
                if (*(u8 *)(r + 0xE) == 1) {
                    *(u16 *)(s0 + 0xB6) = 0;
                    *(u16 *)(s0 + 0xB4) = 0;
                    *(u8 *)(r + 0xE) = 2;
                } else {
                    func_0029F338(s0);
                }
            }
        }
        s0 += 0x100;
    } while (D_L16_001B2B20[0] >= s0);
}
