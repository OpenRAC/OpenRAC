#include "include_asm.h"

typedef unsigned char u8;
typedef float f32;
extern u8 D_001A63A8_4[];
extern u8 D_00189E20[];
extern f32 func_00282E48(void *a0, void *a1);
extern f32 func_002A7878(f32 low, f32 high);
extern void func_0032C700(void *a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                          int a8, int a9, int a10, int a11, f32 f12, f32 f13);

void func_PAL_L22_00413048(u8 *a0) {
    f32 f = func_002A7878(0.8f, 1.2f);
    func_0032C700(a0, 0, 5, 4, 4, 4, 4, 0, 0, 0, 0, 0, f, 20.0f);
}

