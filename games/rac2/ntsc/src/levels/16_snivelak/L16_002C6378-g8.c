#include "include_asm.h"

typedef signed int s32;
typedef unsigned char u8;
typedef float f32;

/* The four words the two passes hand to func_L16_003E7248, all read through $gp. */
extern int D_L16_001B0C9C __attribute__((sda));

extern int D_L16_001B0CA0 __attribute__((sda));

extern int D_L16_001B0CA8 __attribute__((sda));

extern int D_L16_001B0CAC __attribute__((sda));

extern s32 func_002A7820(s32, s32);

extern f32 func_002A7940(void);

extern void func_L16_003E7248(u8 *, int, f32, int, int, f32, f32, int, u8 *);

/* Draws the object's two sprites: each pass rolls a fresh value for its frame range and hands
   the object's position, the rolled value, the two $gp words and the drawer's depth/size
   arguments on. */
void func_L16_003E70E0(u8 *a0) {
    func_L16_003E7248(a0 + 0x10, D_L16_001B0CA8, *(f32 *)(a0 + 0x2C),
                      func_002A7820(0x5F, 0x6F), D_L16_001B0C9C, func_002A7940(),
                      1.0f, 2, a0);
    {
        f32 half = *(f32 *)(a0 + 0x2C) * 0.5f;

        func_L16_003E7248(a0 + 0x10, D_L16_001B0CAC, half,
                          func_002A7820(0x6F, 0x7F), D_L16_001B0CA0,
                          func_002A7940(), 1.0f, 2, a0);
    }
}
