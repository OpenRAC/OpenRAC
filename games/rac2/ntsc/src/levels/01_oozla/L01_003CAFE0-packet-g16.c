#include "common.h"

/* Incomplete views keep absolute macro accesses; the measured 16-byte template
 * uses small data. The packet cursor is reread between packet-field stores. */
extern u8 * volatile D_L01_001B2CC8[] __attribute__((sda));
extern u8 *D_L01_001B2CC8_gp __asm__("D_L01_001B2CC8") MACRO_ADDR;
extern u8 D_L01_001B02A0[16] __attribute__((sda));
extern float D_L01_001B31D4[] __attribute__((sda));
extern float D_L01_001B31D8[] __attribute__((sda));
extern float D_L01_001B31DC[] __attribute__((sda));
extern float D_L01_001B31E0[] __attribute__((sda));
extern u8 D_L01_001BD9C0[];
extern void func_002DA4F8(void);
extern void func_002D96D0(void);
extern void func_00371A70(void);

void func_L01_003CB540(void)
{
    u8 *state;
    u8 *p = D_L01_001B2CC8[0];
    qcopy(p, D_L01_001B02A0);
    p += 16;
    D_L01_001B2CC8[0] = p;
    {
        float a = D_L01_001B31D4[0], b = D_L01_001B31D8[0];
        float c = D_L01_001B31DC[0], d = D_L01_001B31E0[0];
        state = D_L01_001BD9C0;
        *(float *)(state + 0x218) = a;
        *(float *)(state + 0x21C) = b;
        *(float *)(state + 0x228) = c;
        *(float *)(state + 0x22C) = d;
    }
    func_002DA4F8();
    func_002D96D0();
    *(u32 *)(D_L01_001B2CC8[0] + 0) = 0x10000002;
    *(u32 *)(D_L01_001B2CC8[0] + 4) = 0;
    *(u32 *)(D_L01_001B2CC8[0] + 8) = 0x11000000;
    *(u32 *)(D_L01_001B2CC8[0] + 12) = 0x50000002;
    {
        u8 *packet = D_L01_001B2CC8[0];
        D_L01_001B2CC8[0] = packet + 16;
        {
            u8 *next = packet + 16;
            *(u64 *)(packet + 16) = 0x1000000000008001ULL;
            *(u64 *)(next + 8) = 14;
            *(u64 *)(next + 16) = (u64)*(s32 *)(state + 0x230) |
                ((u64)*(s32 *)(state + 0x234) << 8) |
                ((u64)*(s32 *)(state + 0x238) << 16);
            *(u64 *)(next + 24) = 0x3D;
            D_L01_001B2CC8_gp = packet + 48;
        }
    }
    func_00371A70();
}
