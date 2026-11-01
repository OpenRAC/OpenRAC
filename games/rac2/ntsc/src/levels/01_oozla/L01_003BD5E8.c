#include "include_asm.h"
#include "common.h"

typedef int TI128 __attribute__((mode(TI)));
typedef union { u8 b[0x2600]; s16 h[0x1300]; s32 i[0x980]; u32 u[0x980]; f32 f[0x980]; void *ptr[0x980]; TI128 q[0x260]; } Hero;
extern char D_00189EA0[];

typedef struct {
    u8 pad00[0x10];
    f32 pos[4];
    u8 f20;
    u8 pad21[0x68 - 0x21];
    struct Mount *f68;
} Ent;

typedef struct Mount {
    u8 pad00[0x1D8];
    f32 f1D8;
    u8 pad1DC[0x224 - 0x1DC];
    f32 f224;
    f32 f228;
    u8 pad22C[0x27C - 0x22C];
    f32 f27C;
    u8 pad280[8];
    f32 f288;
} Mount;

extern f32 func_L01_003BD560(void *, f32, f32, f32, f32);

/* The mount's floor under the entry, raised to its minimum, and the ceiling over the hero. */
void func_L01_003BD5E8(Ent *a0)
{
    Mount *s0 = a0->f68;
    f32 pos[4] __attribute__((aligned(16)));
    f32 top, hero;
    qcopy(pos, a0->pos);
    if (a0->f20 == 0) {
        s0->f224 = func_L01_003BD560(pos, s0->f1D8 + 20.0f, s0->f1D8 - 10.0f, s0->f27C, s0->f288);
    } else {
        s0->f224 = func_L01_003BD560(pos, a0->pos[2] + 2.0f, a0->pos[2] - 10.0f, s0->f27C, s0->f288);
    }
    if (s0->f224 < s0->f27C) s0->f224 = s0->f27C;
    top = s0->f224 + 1.7f;
    hero = ((Hero *)D_00189EA0)->f[8 / 4] + 0.1f;
    s0->f228 = top > hero ? top : hero;
}
