/* func_L10_0042BCA0 (384 B, 1 place), M31 26 Oct 2026: 2 lines off at the default -G0: one
   store pair in the init arm (retail sb 0x30 before swc1 0x10). 720 store orders and chained
   pos stores tried; queued for the permuter. */
#include "include_asm.h"
#include "common.h"

typedef int TI __attribute__((mode(TI)));

typedef struct {
    u8 pad00[0x24];
    f32 f24;
} Class42BCA0;

typedef struct Moby42BCA0 {
    u8 pad00[0x10];
    f32 pos[4];
    u8 state;
    u8 pad21[3];
    Class42BCA0 *cls;
    u8 pad28[4];
    f32 f2C;
    u8 alpha;
    u8 pad31;
    s16 f32;
    u8 pad34[0x68 - 0x34];
    struct Vars42BCA0 *vars;
    u8 pad6C[0xF0 - 0x6C];
    f32 fF0;
    u8 padF4[4];
    f32 fF8;
} Moby42BCA0;

typedef struct Vars42BCA0 {
    s32 flip;
    Moby42BCA0 *child;
} Vars42BCA0;

extern float func_00283B98(float, float);
extern float func_00283BE0(float, float);
extern Moby42BCA0 *func_0029EFD0(int);

void func_L10_0042BCA0(Moby42BCA0 *self) {
    Vars42BCA0 *vars = self->vars;

    switch (self->state) {
    case 0:
        self->state = 1;
        self->f32 = 0x1FF;
        self->pos[0] = 512.0f;
        self->alpha = 0xFF;
        self->pos[1] = 512.0f;
        self->pos[2] = 512.0f;
        self->pos[3] = 0;
        self->f2C = self->cls->f24 * 4.0f;
        vars->child = func_0029EFD0(0x11CD);
        vars->child->alpha = 0xFF;
        vars->child->f32 = 0x1FF;
        vars->child->f2C = vars->child->cls->f24 * 4.0f;
        if (vars->flip != 0) {
            self->fF0 = 3.1415927f;
            self->pos[2] = self->pos[2] + 6.0f;
        } else {
            self->pos[2] = self->pos[2] - 6.0f;
        }
    case 1:
        if (vars->flip != 0) {
            self->fF8 = func_00283B98(self->fF8, 0.004908739f);
        } else {
            self->fF8 = func_00283BE0(self->fF8, 0.004908739f);
        }
        vars->child->fF0 = self->fF0;
        vars->child->fF8 = self->fF8;
        qcopy(vars->child->pos, self->pos);
        break;
    }
}
